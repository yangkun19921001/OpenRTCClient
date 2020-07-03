/*
 *  Copyright 2020 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "call/adaptation/resource_adaptation_processor.h"

#include <algorithm>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "api/video/video_adaptation_counters.h"
#include "call/adaptation/video_stream_adapter.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/strings/string_builder.h"
#include "rtc_base/task_utils/to_queued_task.h"

namespace webrtc {

ResourceAdaptationProcessor::ResourceListenerDelegate::ResourceListenerDelegate(
    ResourceAdaptationProcessor* processor)
    : resource_adaptation_queue_(nullptr), processor_(processor) {}

void ResourceAdaptationProcessor::ResourceListenerDelegate::
    SetResourceAdaptationQueue(TaskQueueBase* resource_adaptation_queue) {
  RTC_DCHECK(!resource_adaptation_queue_);
  RTC_DCHECK(resource_adaptation_queue);
  resource_adaptation_queue_ = resource_adaptation_queue;
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
}

void ResourceAdaptationProcessor::ResourceListenerDelegate::
    OnProcessorDestroyed() {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  processor_ = nullptr;
}

void ResourceAdaptationProcessor::ResourceListenerDelegate::
    OnResourceUsageStateMeasured(rtc::scoped_refptr<Resource> resource,
                                 ResourceUsageState usage_state) {
  if (!resource_adaptation_queue_->IsCurrent()) {
    resource_adaptation_queue_->PostTask(ToQueuedTask(
        [this_ref = rtc::scoped_refptr<ResourceListenerDelegate>(this),
         resource, usage_state] {
          this_ref->OnResourceUsageStateMeasured(resource, usage_state);
        }));
    return;
  }
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  if (processor_) {
    processor_->OnResourceUsageStateMeasured(resource, usage_state);
  }
}

ResourceAdaptationProcessor::MitigationResultAndLogMessage::
    MitigationResultAndLogMessage()
    : result(MitigationResult::kAdaptationApplied), message() {}

ResourceAdaptationProcessor::MitigationResultAndLogMessage::
    MitigationResultAndLogMessage(MitigationResult result, std::string message)
    : result(result), message(std::move(message)) {}

ResourceAdaptationProcessor::ResourceAdaptationProcessor(
    VideoStreamEncoderObserver* encoder_stats_observer,
    VideoStreamAdapter* stream_adapter)
    : resource_adaptation_queue_(nullptr),
      resource_listener_delegate_(
          new rtc::RefCountedObject<ResourceListenerDelegate>(this)),
      encoder_stats_observer_(encoder_stats_observer),
      resources_(),
      effective_degradation_preference_(DegradationPreference::DISABLED),
      stream_adapter_(stream_adapter),
      last_reported_source_restrictions_(),
      previous_mitigation_results_(),
      processing_in_progress_(false) {
  RTC_DCHECK(stream_adapter_);
}

ResourceAdaptationProcessor::~ResourceAdaptationProcessor() {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(resources_.empty())
      << "There are resource(s) attached to a ResourceAdaptationProcessor "
      << "being destroyed.";
  RTC_DCHECK(adaptation_constraints_.empty())
      << "There are constaint(s) attached to a ResourceAdaptationProcessor "
      << "being destroyed.";
  RTC_DCHECK(adaptation_listeners_.empty())
      << "There are listener(s) attached to a ResourceAdaptationProcessor "
      << "being destroyed.";
  stream_adapter_->RemoveRestrictionsListener(this);
  resource_listener_delegate_->OnProcessorDestroyed();
}

void ResourceAdaptationProcessor::SetResourceAdaptationQueue(
    TaskQueueBase* resource_adaptation_queue) {
  RTC_DCHECK(!resource_adaptation_queue_);
  RTC_DCHECK(resource_adaptation_queue);
  resource_adaptation_queue_ = resource_adaptation_queue;
  resource_listener_delegate_->SetResourceAdaptationQueue(
      resource_adaptation_queue);
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  // Now that we have the adaptation queue we can attach as adaptation listener.
  stream_adapter_->AddRestrictionsListener(this);
}

void ResourceAdaptationProcessor::AddResourceLimitationsListener(
    ResourceLimitationsListener* limitations_listener) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(std::find(resource_limitations_listeners_.begin(),
                       resource_limitations_listeners_.end(),
                       limitations_listener) ==
             resource_limitations_listeners_.end());
  resource_limitations_listeners_.push_back(limitations_listener);
}
void ResourceAdaptationProcessor::RemoveResourceLimitationsListener(
    ResourceLimitationsListener* limitations_listener) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  auto it =
      std::find(resource_limitations_listeners_.begin(),
                resource_limitations_listeners_.end(), limitations_listener);
  RTC_DCHECK(it != resource_limitations_listeners_.end());
  resource_limitations_listeners_.erase(it);
}

void ResourceAdaptationProcessor::AddResource(
    rtc::scoped_refptr<Resource> resource) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(resource);
  RTC_DCHECK(absl::c_find(resources_, resource) == resources_.end())
      << "Resource \"" << resource->Name() << "\" was already registered.";
  resources_.push_back(resource);
  resource->SetResourceListener(resource_listener_delegate_);
}

std::vector<rtc::scoped_refptr<Resource>>
ResourceAdaptationProcessor::GetResources() const {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  return resources_;
}

void ResourceAdaptationProcessor::RemoveResource(
    rtc::scoped_refptr<Resource> resource) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(resource);
  RTC_LOG(INFO) << "Removing resource \"" << resource->Name() << "\".";
  auto it = absl::c_find(resources_, resource);
  RTC_DCHECK(it != resources_.end()) << "Resource \"" << resource->Name()
                                     << "\" was not a registered resource.";
  auto resource_adaptation_limits =
      adaptation_limits_by_resources_.find(resource);
  if (resource_adaptation_limits != adaptation_limits_by_resources_.end()) {
    VideoStreamAdapter::RestrictionsWithCounters adaptation_limits =
        resource_adaptation_limits->second;
    adaptation_limits_by_resources_.erase(resource_adaptation_limits);
    MaybeUpdateResourceLimitationsOnResourceRemoval(adaptation_limits);
  }
  resources_.erase(it);
  resource->SetResourceListener(nullptr);
}

void ResourceAdaptationProcessor::AddAdaptationConstraint(
    AdaptationConstraint* adaptation_constraint) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(std::find(adaptation_constraints_.begin(),
                       adaptation_constraints_.end(),
                       adaptation_constraint) == adaptation_constraints_.end());
  adaptation_constraints_.push_back(adaptation_constraint);
}

void ResourceAdaptationProcessor::RemoveAdaptationConstraint(
    AdaptationConstraint* adaptation_constraint) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  auto it = std::find(adaptation_constraints_.begin(),
                      adaptation_constraints_.end(), adaptation_constraint);
  RTC_DCHECK(it != adaptation_constraints_.end());
  adaptation_constraints_.erase(it);
}

void ResourceAdaptationProcessor::AddAdaptationListener(
    AdaptationListener* adaptation_listener) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(std::find(adaptation_listeners_.begin(),
                       adaptation_listeners_.end(),
                       adaptation_listener) == adaptation_listeners_.end());
  adaptation_listeners_.push_back(adaptation_listener);
}

void ResourceAdaptationProcessor::RemoveAdaptationListener(
    AdaptationListener* adaptation_listener) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  auto it = std::find(adaptation_listeners_.begin(),
                      adaptation_listeners_.end(), adaptation_listener);
  RTC_DCHECK(it != adaptation_listeners_.end());
  adaptation_listeners_.erase(it);
}

void ResourceAdaptationProcessor::OnDegradationPreferenceUpdated(
    DegradationPreference degradation_preference) {
  if (!resource_adaptation_queue_->IsCurrent()) {
    resource_adaptation_queue_->PostTask(
        ToQueuedTask([this, degradation_preference]() {
          OnDegradationPreferenceUpdated(degradation_preference);
        }));
    return;
  }
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  if (degradation_preference == effective_degradation_preference_) {
    return;
  }
  effective_degradation_preference_ = degradation_preference;
  stream_adapter_->SetDegradationPreference(effective_degradation_preference_);
}

void ResourceAdaptationProcessor::OnResourceUsageStateMeasured(
    rtc::scoped_refptr<Resource> resource,
    ResourceUsageState usage_state) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(resource);
  // |resource| could have been removed after signalling.
  if (absl::c_find(resources_, resource) == resources_.end()) {
    RTC_LOG(INFO) << "Ignoring signal from removed resource \""
                  << resource->Name() << "\".";
    return;
  }
  MitigationResultAndLogMessage result_and_message;
  switch (usage_state) {
    case ResourceUsageState::kOveruse:
      result_and_message = OnResourceOveruse(resource);
      break;
    case ResourceUsageState::kUnderuse:
      result_and_message = OnResourceUnderuse(resource);
      break;
  }
  // Maybe log the result of the operation.
  auto it = previous_mitigation_results_.find(resource.get());
  if (it != previous_mitigation_results_.end() &&
      it->second == result_and_message.result) {
    // This resource has previously reported the same result and we haven't
    // successfully adapted since - don't log to avoid spam.
    return;
  }
  RTC_LOG(INFO) << "Resource \"" << resource->Name() << "\" signalled "
                << ResourceUsageStateToString(usage_state) << ". "
                << result_and_message.message;
  if (result_and_message.result == MitigationResult::kAdaptationApplied) {
    previous_mitigation_results_.clear();
  } else {
    previous_mitigation_results_.insert(
        std::make_pair(resource.get(), result_and_message.result));
  }
}

ResourceAdaptationProcessor::MitigationResultAndLogMessage
ResourceAdaptationProcessor::OnResourceUnderuse(
    rtc::scoped_refptr<Resource> reason_resource) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(!processing_in_progress_);
  processing_in_progress_ = true;
  if (effective_degradation_preference_ == DegradationPreference::DISABLED) {
    processing_in_progress_ = false;
    return MitigationResultAndLogMessage(
        MitigationResult::kDisabled,
        "Not adapting up because DegradationPreference is disabled");
  }
  // How can this stream be adapted up?
  Adaptation adaptation = stream_adapter_->GetAdaptationUp();
  if (adaptation.status() != Adaptation::Status::kValid) {
    processing_in_progress_ = false;
    rtc::StringBuilder message;
    message << "Not adapting up because VideoStreamAdapter returned "
            << Adaptation::StatusToString(adaptation.status());
    return MitigationResultAndLogMessage(MitigationResult::kRejectedByAdapter,
                                         message.Release());
  }
  VideoSourceRestrictions restrictions_before =
      stream_adapter_->source_restrictions();
  VideoStreamAdapter::RestrictionsWithCounters peek_restrictions =
      stream_adapter_->PeekNextRestrictions(adaptation);
  VideoSourceRestrictions restrictions_after = peek_restrictions.restrictions;
  // Check that resource is most limited...
  std::vector<rtc::scoped_refptr<Resource>> most_limited_resources;
  VideoStreamAdapter::RestrictionsWithCounters most_limited_restrictions;
  std::tie(most_limited_resources, most_limited_restrictions) =
      FindMostLimitedResources();

  for (const auto* constraint : adaptation_constraints_) {
    if (!constraint->IsAdaptationUpAllowed(
            adaptation.input_state(), restrictions_before, restrictions_after,
            reason_resource)) {
      processing_in_progress_ = false;
      rtc::StringBuilder message;
      message << "Not adapting up because constraint \"" << constraint->Name()
              << "\" disallowed it";
      return MitigationResultAndLogMessage(
          MitigationResult::kRejectedByConstraint, message.Release());
    }
  }
  // If the most restricted resource is less limited than current restrictions
  // then proceed with adapting up.
  if (!most_limited_resources.empty() &&
      most_limited_restrictions.adaptation_counters.Total() >=
          stream_adapter_->adaptation_counters().Total()) {
    // If |reason_resource| is not one of the most limiting resources then abort
    // adaptation.
    if (absl::c_find(most_limited_resources, reason_resource) ==
        most_limited_resources.end()) {
      processing_in_progress_ = false;
      rtc::StringBuilder message;
      message << "Resource \"" << reason_resource->Name()
              << "\" was not the most limited resource.";
      return MitigationResultAndLogMessage(
          MitigationResult::kNotMostLimitedResource, message.Release());
    }

    if (most_limited_resources.size() > 1) {
      // If there are multiple most limited resources, all must signal underuse
      // before the adaptation is applied.
      UpdateResourceLimitations(reason_resource, peek_restrictions);
      processing_in_progress_ = false;
      rtc::StringBuilder message;
      message << "Resource \"" << reason_resource->Name()
              << "\" was not the only most limited resource.";
      return MitigationResultAndLogMessage(
          MitigationResult::kSharedMostLimitedResource, message.Release());
    }
  }
  // Apply adaptation.
  stream_adapter_->ApplyAdaptation(adaptation, reason_resource);
  for (auto* adaptation_listener : adaptation_listeners_) {
    adaptation_listener->OnAdaptationApplied(
        adaptation.input_state(), restrictions_before, restrictions_after,
        reason_resource);
  }
  processing_in_progress_ = false;
  rtc::StringBuilder message;
  message << "Adapted up successfully. Unfiltered adaptations: "
          << stream_adapter_->adaptation_counters().ToString();
  return MitigationResultAndLogMessage(MitigationResult::kAdaptationApplied,
                                       message.Release());
}

ResourceAdaptationProcessor::MitigationResultAndLogMessage
ResourceAdaptationProcessor::OnResourceOveruse(
    rtc::scoped_refptr<Resource> reason_resource) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_DCHECK(!processing_in_progress_);
  processing_in_progress_ = true;
  if (effective_degradation_preference_ == DegradationPreference::DISABLED) {
    processing_in_progress_ = false;
    return MitigationResultAndLogMessage(
        MitigationResult::kDisabled,
        "Not adapting down because DegradationPreference is disabled");
  }
  // How can this stream be adapted up?
  Adaptation adaptation = stream_adapter_->GetAdaptationDown();
  if (adaptation.min_pixel_limit_reached()) {
    encoder_stats_observer_->OnMinPixelLimitReached();
  }
  if (adaptation.status() != Adaptation::Status::kValid) {
    processing_in_progress_ = false;
    rtc::StringBuilder message;
    message << "Not adapting down because VideoStreamAdapter returned "
            << Adaptation::StatusToString(adaptation.status());
    return MitigationResultAndLogMessage(MitigationResult::kRejectedByAdapter,
                                         message.Release());
  }
  // Apply adaptation.
  VideoSourceRestrictions restrictions_before =
      stream_adapter_->source_restrictions();
  VideoStreamAdapter::RestrictionsWithCounters peek_next_restrictions =
      stream_adapter_->PeekNextRestrictions(adaptation);
  VideoSourceRestrictions restrictions_after =
      peek_next_restrictions.restrictions;
  stream_adapter_->ApplyAdaptation(adaptation, reason_resource);
  for (auto* adaptation_listener : adaptation_listeners_) {
    adaptation_listener->OnAdaptationApplied(
        adaptation.input_state(), restrictions_before, restrictions_after,
        reason_resource);
  }
  processing_in_progress_ = false;
  rtc::StringBuilder message;
  message << "Adapted down successfully. Unfiltered adaptations: "
          << stream_adapter_->adaptation_counters().ToString();
  return MitigationResultAndLogMessage(MitigationResult::kAdaptationApplied,
                                       message.Release());
}

void ResourceAdaptationProcessor::TriggerAdaptationDueToFrameDroppedDueToSize(
    rtc::scoped_refptr<Resource> reason_resource) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  RTC_LOG(INFO) << "TriggerAdaptationDueToFrameDroppedDueToSize called";
  VideoAdaptationCounters counters_before =
      stream_adapter_->adaptation_counters();
  OnResourceOveruse(reason_resource);
  if (effective_degradation_preference_ == DegradationPreference::BALANCED &&
      stream_adapter_->adaptation_counters().fps_adaptations >
          counters_before.fps_adaptations) {
    // Oops, we adapted frame rate. Adapt again, maybe it will adapt resolution!
    // Though this is not guaranteed...
    OnResourceOveruse(reason_resource);
  }
  if (stream_adapter_->adaptation_counters().resolution_adaptations >
      counters_before.resolution_adaptations) {
    encoder_stats_observer_->OnInitialQualityResolutionAdaptDown();
  }
}

std::pair<std::vector<rtc::scoped_refptr<Resource>>,
          VideoStreamAdapter::RestrictionsWithCounters>
ResourceAdaptationProcessor::FindMostLimitedResources() const {
  std::vector<rtc::scoped_refptr<Resource>> most_limited_resources;
  VideoStreamAdapter::RestrictionsWithCounters most_limited_restrictions{
      VideoSourceRestrictions(), VideoAdaptationCounters()};

  for (const auto& resource_and_adaptation_limit_ :
       adaptation_limits_by_resources_) {
    const auto& restrictions_with_counters =
        resource_and_adaptation_limit_.second;
    if (restrictions_with_counters.adaptation_counters.Total() >
        most_limited_restrictions.adaptation_counters.Total()) {
      most_limited_restrictions = restrictions_with_counters;
      most_limited_resources.clear();
      most_limited_resources.push_back(resource_and_adaptation_limit_.first);
    } else if (most_limited_restrictions.adaptation_counters ==
               restrictions_with_counters.adaptation_counters) {
      most_limited_resources.push_back(resource_and_adaptation_limit_.first);
    }
  }
  return std::make_pair(std::move(most_limited_resources),
                        most_limited_restrictions);
}

void ResourceAdaptationProcessor::UpdateResourceLimitations(
    rtc::scoped_refptr<Resource> reason_resource,
    const VideoStreamAdapter::RestrictionsWithCounters& restrictions) {
  auto& adaptation_limits = adaptation_limits_by_resources_[reason_resource];
  if (adaptation_limits == restrictions) {
    return;
  }
  adaptation_limits = restrictions;

  std::map<rtc::scoped_refptr<Resource>, VideoAdaptationCounters> limitations;
  for (const auto& p : adaptation_limits_by_resources_) {
    limitations.insert(std::make_pair(p.first, p.second.adaptation_counters));
  }
  for (auto limitations_listener : resource_limitations_listeners_) {
    limitations_listener->OnResourceLimitationChanged(reason_resource,
                                                      limitations);
  }
}

void ResourceAdaptationProcessor::
    MaybeUpdateResourceLimitationsOnResourceRemoval(
        VideoStreamAdapter::RestrictionsWithCounters removed_limitations) {
  if (adaptation_limits_by_resources_.empty()) {
    // Only the resource being removed was adapted so clear restrictions.
    stream_adapter_->ClearRestrictions();
    return;
  }

  VideoStreamAdapter::RestrictionsWithCounters most_limited =
      FindMostLimitedResources().second;

  if (removed_limitations.adaptation_counters.Total() <=
      most_limited.adaptation_counters.Total()) {
    // The removed limitations were less limited than the most limited resource.
    // Don't change the current restrictions.
    return;
  }

  // Apply the new most limited resource as the next restrictions.
  Adaptation adapt_to = stream_adapter_->GetAdaptationTo(
      most_limited.adaptation_counters, most_limited.restrictions);
  RTC_DCHECK_EQ(adapt_to.status(), Adaptation::Status::kValid);
  stream_adapter_->ApplyAdaptation(adapt_to, nullptr);
  for (auto* adaptation_listener : adaptation_listeners_) {
    adaptation_listener->OnAdaptationApplied(
        adapt_to.input_state(), removed_limitations.restrictions,
        most_limited.restrictions, nullptr);
  }

  RTC_LOG(INFO) << "Most limited resource removed. Restoring restrictions to "
                   "next most limited restrictions: "
                << most_limited.restrictions.ToString() << " with counters "
                << most_limited.adaptation_counters.ToString();
}

void ResourceAdaptationProcessor::OnVideoSourceRestrictionsUpdated(
    VideoSourceRestrictions restrictions,
    const VideoAdaptationCounters& adaptation_counters,
    rtc::scoped_refptr<Resource> reason,
    const VideoSourceRestrictions& unfiltered_restrictions) {
  RTC_DCHECK_RUN_ON(resource_adaptation_queue_);
  if (reason) {
    UpdateResourceLimitations(reason,
                              {unfiltered_restrictions, adaptation_counters});
  } else if (adaptation_counters.Total() == 0) {
    // Adaptations are cleared.
    adaptation_limits_by_resources_.clear();
    previous_mitigation_results_.clear();
    for (auto limitations_listener : resource_limitations_listeners_) {
      limitations_listener->OnResourceLimitationChanged(nullptr, {});
    }
  }
}

}  // namespace webrtc
