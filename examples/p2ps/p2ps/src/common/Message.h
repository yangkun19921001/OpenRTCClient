#ifndef MESSAGE_H
#define MESSAGE_H
#include <memory>
namespace PCS {

enum class RTCMainEvent {
    NONE = 0,
    CONNECT,
    JOIN,
    LEAVE,
    ON_JOINED,
    ON_LEAVED,
    ON_OFFER,
    ON_ANSWER,
    ON_CANDIDATE,
    ON_ADD_LOCAL_TRACK,
    ON_ADD_REMOTE_TRACK,
    ON_RELEASE,
};

struct Message
{
    Message() {}
    RTCMainEvent what = RTCMainEvent::NONE;
    std::shared_ptr <void> data;
    std::shared_ptr <void> data_1;
    std::shared_ptr <void> data_2;
    std::shared_ptr <void> data_3;
    std::shared_ptr <void> data_4;
    std::shared_ptr <void> data_5;
    std::shared_ptr <void> data_6;
    std::shared_ptr <void> data_7;

    int64_t nsptr_data;

};

}
#endif // MESSAGE_H
