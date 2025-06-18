"""
Chat Room message formats and functions for sending and
receiving messages from the chat room server.
"""

import struct

from vardorvis_cmd.vardorvis_cmd import VardorvisCmd

# ERR Return
ERR_RETURN = -1
EMPTY_RETURN = -2

# packet types
ROOMS_TYPE = 0
ACCOUNT_TYPE = 1
CHAT_TYPE = 2
SESSION_TYPE = 3
FAIL_TYPE = 255

# packet sub types
JOIN_STYPE = 0
LIST_STYPE = 1
CREATE_STYPE = 2
REGISTER_STYPE = 3
LOGIN_STYPE = 4
ADMIN_STYPE = 5
CHAT_STYPE = 6
FAIL_STYPE = 7
DEL_STYPE = 8
ADMIN_REMOVE_STYPE = 9
LEAVE_STYPE = 10
LOGOUT_STYPE = 11
QUIT_STYPE = 12

# opcodes
REQUEST = 0
RESPONSE = 1
REJECT = 2
ACKNOWLEDGE = 3

# Reject codes
SRV_BUSY_RCODE = 0
SRV_ERR_RCODE = 1
INVALID_PACKET_RCODE = 2
USER_NAME_LEN = 3
USER_NAME_CHAR = 4
PASS_LEN = 5
PASS_CHAR = 6
USER_DOES_NOT_EXIST = 7
INCORRECT_PASS = 8
ADMIN_PRIV = 9
USER_EXISTS = 10
ROOM_EXISTS = 11
USER_LOGGED_IN = 12
ADMIN_SELF = 13
MAX_USERS = 14
MAX_CLIENTS = 15
MAX_ROOMS = 16
NO_ROOMS = 17
ROOM_LEN = 18
ROOM_CHARS = 19
RESERVED_ROOM_NAME = 20
ROOM_DOES_NOT_EXIST = 21
ROOM_IN_USE = 22

REASON_CODE_MSG = {
    SRV_BUSY_RCODE: "Server Busy",
    SRV_ERR_RCODE: "Server Error",
    INVALID_PACKET_RCODE: "Invalid packet received by server",
    USER_NAME_LEN: "Username out of range",
    USER_NAME_CHAR: "Username has invalid characters, try again",
    PASS_LEN: "Password out of range",
    PASS_CHAR: "Password has invalid characters, try again",
    USER_DOES_NOT_EXIST: "User does not exist",
    INCORRECT_PASS: "Incorrect password",
    ADMIN_PRIV: "Requires admin privileges",
    USER_EXISTS: "User already exists",
    ROOM_EXISTS: "Room already exists",
    USER_LOGGED_IN: "User is already logged in",
    ADMIN_SELF: "You can't update your own admin status/delete your own account",
    MAX_USERS: "The server has reached its maximum number of users",
    MAX_CLIENTS: "The server has reached its maximum number of clients",
    NO_ROOMS: "The server does not currently have any rooms",
    MAX_ROOMS: "The server has reached its maximum number of rooms",
    ROOM_LEN: "Room name length out of range",
    ROOM_CHARS: "Room has invalid characters, try again",
    RESERVED_ROOM_NAME: "Room name is reserved",
    ROOM_DOES_NOT_EXIST: "Room does not exist",
    ROOM_IN_USE: "Room currently in use",
}

# Maximum lengths for username, password, room name and chat length.
MAX_NAME_LEN = 30
MAX_CHAT_LEN = 150

# Any received message will have the first three bytes that determine packet type, sub-type,
# and opcode.
RECVD_MSG = "BBB"
RECVD_REJ = "BBBB"

# Register type packets.
REGISTER_REQ = "BBB31s31s"  # ACCOUNT_TYPE, REGISTER_STYPE, REQUEST, username, password

BUFF_SIZE = 1024
MESG_SIZE = 4


def register_req_create(username: str, password: str) -> bytes:
    """
    Create a byte array that meets register request packet format.
    """
    if len(username) > MAX_NAME_LEN or len(password) > MAX_NAME_LEN:
        raise NameError("Username and password must be 30 characters or less.")

    mesg_username = username.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    mesg_password = password.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_password = mesg_password.encode("UTF-8")

    message = (ACCOUNT_TYPE, REGISTER_STYPE, REQUEST, bytes_username, bytes_password)

    return struct.pack(REGISTER_REQ, *message)


# Delete user packets.
DELETE_REQ = "BBB31s"  # ACCOUNT_TYPE, DEL_STYPE, REQUEST


def user_delete_req_create(username: str) -> bytes:
    """
    Create a byte array that meets user delete request packet format.
    """
    if len(username) > MAX_NAME_LEN:
        raise NameError("Username must be 30 characters or less.")

    mesg_username = username.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ACCOUNT_TYPE, DEL_STYPE, REQUEST, bytes_username)

    return struct.pack(DELETE_REQ, *message)


# Login packets.
LOGIN_REQ = "BBB31s31s"  # ACCOUNT_TYPE, LOGIN_STYPE, REQUEST


def login_req_create(username: str, password: str) -> bytes:
    """
    Create a byte array that meets login request packet format.
    """
    if len(username) > MAX_NAME_LEN or len(password) > MAX_NAME_LEN:
        raise NameError("Username and password must be 30 characters or less.")

    mesg_username = username.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    mesg_password = password.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_password = mesg_password.encode("UTF-8")

    message = (ACCOUNT_TYPE, LOGIN_STYPE, REQUEST, bytes_username, bytes_password)

    return struct.pack(LOGIN_REQ, *message)


# Logout packets.
LOGOUT_REQ = "BBB"  # ACCOUNT_TYPE, LOGOUT_STYPE, REQUEST


def logout_req_create() -> bytes:
    """
    Create a byte array that meets logout request packet format.
    """

    message = (ACCOUNT_TYPE, LOGOUT_STYPE, REQUEST)

    return struct.pack(LOGOUT_REQ, *message)


# Admin packets.
ADMIN_REQ = "BBB31s"  # ACCOUNT_TYPE, ADMIN_STYPE, REQUEST


def admin_req_create(username: str) -> bytes:
    """
    Create a byte array that meets admin request packet format.
    """
    if len(username) > MAX_NAME_LEN:
        raise NameError("Username must be 30 characters or less.")

    mesg_username = username.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ACCOUNT_TYPE, ADMIN_STYPE, REQUEST, bytes_username)

    return struct.pack(ADMIN_REQ, *message)


# Admin priviledge remove packets.
ADMIN_R_REQ = "BBB31s"  # ACCOUNT_TYPE, ADMIN_REMOVE_STYPE, REQUEST


def admin_r_req_create(username: str) -> bytes:
    """
    Create a byte array that meets admin priviledge remove request packet format.
    """
    if len(username) > MAX_NAME_LEN:
        raise NameError("Username must be 30 characters or less.")

    mesg_username = username.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ACCOUNT_TYPE, ADMIN_REMOVE_STYPE, REQUEST, bytes_username)

    return struct.pack(ADMIN_R_REQ, *message)


# Room create packets.
ROOM_REQ = "BBB31s"  # ROOMS_TYPE, CREATE_STYPE, REQUEST


def room_req_create(room_name: str) -> bytes:
    """
    Create a byte array that meets room create request packet format.
    """
    if len(room_name) > MAX_NAME_LEN:
        raise NameError("Room name must be 30 characters or less.")

    mesg_username = room_name.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ROOMS_TYPE, CREATE_STYPE, REQUEST, bytes_username)

    return struct.pack(ROOM_REQ, *message)


# Room delete packets.
ROOM_REQ = "BBB31s"  # ROOMS_TYPE, CREATE_STYPE, REQUEST


def room_del_req_create(room_name: str) -> bytes:
    """
    Create a byte array that meets room create request packet format.
    """
    if len(room_name) > MAX_NAME_LEN:
        raise NameError("Room name must be 30 characters or less.")

    mesg_username = room_name.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ROOMS_TYPE, DEL_STYPE, REQUEST, bytes_username)

    return struct.pack(ROOM_REQ, *message)


# Room list packets.
LIST_REQ = "BBB"  # ROOMS_TYPE, LIST_STYPE, REQUEST


def list_req_create() -> bytes:
    """
    Create a byte array that meets room list request packet format.
    """

    message = (ROOMS_TYPE, LIST_STYPE, REQUEST)

    return struct.pack(LIST_REQ, *message)


# Room join packets.
JOIN_REQ = "BBB31s"  # ROOMS_TYPE, JOIN_STYPE, REQUEST


def join_req_create(room_name: str) -> bytes:
    """
    Create a byte array that meets room join request packet format.
    """
    if len(room_name) > MAX_NAME_LEN:
        raise NameError("Room name must be 30 characters or less.")

    mesg_username = room_name.ljust((MAX_NAME_LEN + 1), "\0")
    bytes_username = mesg_username.encode("UTF-8")

    message = (ROOMS_TYPE, JOIN_STYPE, REQUEST, bytes_username)

    return struct.pack(JOIN_REQ, *message)


# Chat packets.
CHAT_REQ = "BBB151s"  # CHAT_TYPE, CHAT_STYPE, REQUEST. more data will follow with chat.


def chat_req_create(chat: str) -> bytes:
    """
    Create a byte array that meets chat request packet format.
    """
    if len(chat) > MAX_CHAT_LEN:
        raise NameError("Chat must be 150 characters or less.")

    mesg_chat = chat.ljust((MAX_CHAT_LEN + 1), "\0")
    bytes_chat = mesg_chat.encode("UTF-8")

    message = (CHAT_TYPE, CHAT_STYPE, REQUEST, bytes_chat)

    return struct.pack(CHAT_REQ, *message)


# Chat leave packets.
LEAVE_REQ = "BBB"  # CHAT_TYPE, LEAVE_STYPE, REQUEST.


def leave_req_create() -> bytes:
    """
    Create a byte array that meets chat request packet format.
    """
    message = (CHAT_TYPE, LEAVE_STYPE, REQUEST)

    return struct.pack(LEAVE_REQ, *message)


# Quit packets.
LEAVE_REQ = "BBB"  # SESSION_TYPE, QUIT_STYPE, REQUEST.


def quit_req_create() -> bytes:
    """
    Create a byte array that meets chat request packet format.
    """
    message = (SESSION_TYPE, QUIT_STYPE, REQUEST)

    return struct.pack(LEAVE_REQ, *message)


# Failure packet.
FAIL_REJ = "BBBB"  # FAIL_TYPE, FAIL_STYPE, REJECT, reason code


def print_reason_code(reason_code: int, vardorvis_cmd: VardorvisCmd):
    """
    Prints the reason code to the terminal.
    """
    if reason_code in REASON_CODE_MSG:
        vardorvis_cmd.verror(REASON_CODE_MSG[reason_code])
    else:
        vardorvis_cmd.verror("Reject code not recognized.")


def cr_recv_unpack(received_messsage: bytes, vardorvis_cmd: VardorvisCmd):
    """
    Unpacks a message received by the chat room client.
    """
    try:
        packet_type, subtype, opcode = struct.unpack(RECVD_MSG, received_messsage[:3])
    except struct.error:
        return ERR_RETURN, ERR_RETURN, ERR_RETURN, ERR_RETURN
    except TypeError:
        return ERR_RETURN, ERR_RETURN, ERR_RETURN, ERR_RETURN

    if REJECT != opcode:
        return packet_type, subtype, opcode, EMPTY_RETURN

    try:
        packet_type, subtype, opcode, reason_code = struct.unpack(
            RECVD_REJ, received_messsage[:4]
        )
    except struct.error:
        return ERR_RETURN, ERR_RETURN, ERR_RETURN, ERR_RETURN
    except TypeError:
        return ERR_RETURN, ERR_RETURN, ERR_RETURN, ERR_RETURN

    try:
        print_reason_code(reason_code, vardorvis_cmd)
    except TypeError as error:
        vardorvis_cmd.verror(f"cr_recv_unpack: received message, print_reason_code: {error}")

    return packet_type, subtype, opcode, reason_code


# End of _cr_messages.py file
