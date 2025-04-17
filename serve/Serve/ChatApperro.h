#ifndef CHATAPPERRO_H
#define CHATAPPERRO_H

#include <string>
enum class login_status {
    login_success = 0,
    login_failed = 1,
    login_failed_user_not_exist = 2,
    login_failed_password_error = 3,
    login_failed_user_already_login = 4,
};

enum class register_status {
    register_success = 0,
    register_failed = 1,
    register_failed_user_already_exist = 2,
};

enum class change_password_status {
    change_password_success = 0,
    change_password_failed = 1,
    change_password_failed_old_password_error = 2,
};

enum class delete_user_status {
    delete_user_success = 0,
    delete_user_failed = 1,
};

enum class forward_msg_status {
    forward_msg_success = 0,
    forward_msg_failed = 1,
    forward_msg_failed_user_not_online = 2,
};

enum class instruction_status {
    instruction_success = 0,
    instruction_failed = 1,
};


#endif