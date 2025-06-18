"""_utility.py file contains classes and functions that support client functionality."""

# I don't want to refactor more
# pylint: disable=too-many-instance-attributes

import argparse
import socket
import ssl

from vardorvis_cmd.vardorvis_cmd import VardorvisCmd

from chatroomclient import _cr_messages

CONNECTED = 1
NOT_CONNECTED = 0
LOGGED_IN = 1
NOT_LOGGED_IN = 0
CHATTING = 1
NOT_CHATTING = 0


class ChatRoomState:
    """
    Holds state information for the chat room client.
    Contains username and password on assignment.
    """

    def __init__(self, ssl_socket: ssl.SSLSocket):
        self.connection_status = CONNECTED
        self.logged_in_status = NOT_LOGGED_IN
        self.chatting_status = NOT_CHATTING
        self.username = ""
        self.room = ""
        self.last_msg_recvd = ""
        self.chat_history = ""
        self.quit_leave_notifcation = None
        self.ssl_socket = ssl_socket
        self.socket_cond = None
        self.cr_state_cond = None
        self.terminal_cond = None

    def test_connection(self) -> bool:
        """
        Returns a boolean describing if the state is connected or not.
        """
        error_code = self.ssl_socket.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)

        if self.connection_status == CONNECTED and error_code == 0:
            return True

        return False

    def login(self, username: str):
        """
        Updates the logged in status to LOGGED_IN.
        Sets username.
        """
        self.logged_in_status = LOGGED_IN
        self.username = username

    def logout(self):
        """
        Updates the logged in status to NOT_LOGGED_IN.
        Resets username to an empty string
        """
        self.logged_in_status = NOT_LOGGED_IN
        self.username = ""

    def test_login(self) -> bool:
        """
        Returns a boolean describing if the state is logged in or not.
        """
        if self.logged_in_status == LOGGED_IN and self.test_connection():
            return True

        return False

    def enter_room(self, room: str, chat_history: str):
        """
        Updates the chatting status to CHATTING.
        Sets room name.
        """
        self.chatting_status = CHATTING
        self.room = room
        self.chat_history = chat_history

    def leave_room(self):
        """
        Updates the chatting status to NOT_CHATTING.
        Sets room name to an empty string.
        """
        self.chatting_status = NOT_CHATTING
        self.room = ""

    def test_chatting(self) -> bool:
        """
        Returns a boolean describing if the state is chatting or not.
        """
        if self.chatting_status == CHATTING and self.test_connection():
            return True

        return False


def cr_get_addr():
    """
    Retrieves command line arguments for IP and hostanme
    """
    parser = argparse.ArgumentParser()

    parser.add_argument("-i", type=str, required=True)
    parser.add_argument("-p", type=int, required=True)

    args = parser.parse_args()

    return args.i, args.p


def cr_handle_not_acks(
    packet,
    expected_type: int,
    expected_sub_type: int,
    cr_state: ChatRoomState,
    vardorvis_cmd: VardorvisCmd,
):
    """
    Handles packets received from the server that are not acknowledge packets.
    Inputs: packet tuple contains: (packet_type:int, sub_type:int, opcode:int, reason_code:int)
    """
    # NOTE: If any of the header is not accurate, then the packet from the server is probably
    # malformed. The user is notified that this is abnormal behavior and should ensure the client
    # version is correct. This does not necessitate shutdown but should be uncommon. This also
    # checks if the reason code is ERR_RETURN (packet wasn't 4 bytes or more) or
    # INVALID_PACKET_RCODE (the server had received an invalid packet).
    if (
        packet[0] != expected_type
        or packet[1] != expected_sub_type
        or packet[2] != _cr_messages.REJECT
        or packet[3] in {_cr_messages.ERR_RETURN, _cr_messages.INVALID_PACKET_RCODE}
    ):
        if cr_state.test_connection():
            vardorvis_cmd.verror(
                "Invalid packet received from server. Please try again. If the issues persists,"
                " consider upgrading to the newest version of the client."
            )
        else:
            vardorvis_cmd.verror(
                "Server disconnected, client shutting down. Please try again later."
            )
            cr_state.disconnect()


# End of _utility.py file
