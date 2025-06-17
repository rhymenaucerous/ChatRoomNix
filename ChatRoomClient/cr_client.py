"""Module for custom chat client."""

# cmd2 is a larger library and using it can require a lot of attributes
# cmd2 require unused arguments for do_ fns
# pylint: disable=too-many-instance-attributes, unused-argument

# mutex is used to lock the socket for sending and receiving within precmd and
# postcmd, meaning it can't use a context manager
# pylint: disable=consider-using-with

import getpass
import select
import socket
import ssl
import threading

import cmd2
from termcolor import colored
from vardorvis_cmd.vardorvis_cmd import VardorvisCmd

from ChatRoomClient import _cr_messages, _cr_utility

ASCII_ART = """
   ________          __     ____
  / ____/ /_  ____ _/ /_   / __ \\____  ____  ____ ___
 / /   / __ \\/ __ `/ __/  / /_/ / __ \\/ __ \\/ __ `__ \\
/ /___/ / / / /_/ / /_   / _, _/ /_/ / /_/ / / / / / /
\\____/_/ /_/\\__,_/\\__/  /_/ |_|\\____/\\____/_/ /_/ /_/

"""
CATEGORY_CONNECTED = "Chat Room Commands: Connected"
CATEGORY_AUTHENTICATED = "Chat Room Commands: Authenticated"
CATEGORY_CHATTING = "Chat Room Commands: Chatting"


class ChatRoomClient(VardorvisCmd):
    """
    Chat Room Client class.
    """

    def __init__(self, hostname: str = "127.0.0.1", port: int = 1234):
        super().__init__()
        self.intro = colored(ASCII_ART, "yellow", attrs=["bold"])
        self.prompt = colored("[NOT LOGGED IN]> ", "yellow", attrs=["bold"])

        # NOTE: This is used to monitor the socket for disconnection.
        self.monitor_thread = None
        self.monitor_running = False
        self.ssl_mutex = threading.Lock()
        self.shutdown_event = threading.Event()

        self.chatting_event = threading.Event()

        # NOTE: All sends and recvs will be beholden to this timeout. If your network
        # latency is worse than this, you should consider using email instead.
        self.server_addr = (hostname, port)

        # WARNING: The socket does not check the hostname and doesn't check for a valid certificate.
        # This is done as the chat room server has a self-signed certificate and will not pass the
        # verification process. A vlid certificate would need to be acquired by registering the
        # server.
        self.ssl_context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
        self.ssl_context.check_hostname = False  # Enable hostname verification
        self.ssl_context.verify_mode = ssl.CERT_NONE
        self.register_precmd_hook(self._pre_cmd_lock)
        self.register_postcmd_hook(self._post_cmd_release)

        try:
            self.connect()
            self.ssl_socket.settimeout(5)
            self.start_socket_monitor()
        except (OSError, ssl.SSLError) as error:
            self.verror(f"Error connecting to the server: {error}")
            raise

        # NOTE: This is used to track the state of the client.
        self.cr_state = _cr_utility.ChatRoomState(self.ssl_socket)

    def connect(self):
        """
        Connect to the chat room server.
        """
        # socket() default: socket.AF_INET, socket.SOCK_STREAM
        self.server_socket = socket.create_connection(self.server_addr)
        self.ssl_socket = self.ssl_context.wrap_socket(self.server_socket)
        self.ssl_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    def _monitor_socket(self):
        """
        Monitor the socket for disconnection using select.
        """

        def _handle_chats():
            """
            Handle chat from the server.
            """

            def _format_line(message: str):
                username, chat_text = message.split(">", 1)
                prompt_str = f"[{username}@{self.cr_state.room}]>"
                colored_prompt = colored(prompt_str, "cyan", attrs=["bold"])
                return f"{colored_prompt}{chat_text}"

            received_messsage = self.ssl_socket.recv(
                _cr_messages.MESG_SIZE + _cr_messages.BUFF_SIZE
            )
            packet_type, subtype, opcode, _ = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.CHAT_TYPE
                and subtype == _cr_messages.CHAT_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):

                # NOTE: It's possible for the server to resend a message and this block ensures
                # that the same message is not printed more than once.
                if self.cr_state.last_msg_recvd != received_messsage[3:].decode(
                    "UTF-8"
                ):
                    self.cr_state.last_msg_recvd = received_messsage[3:].decode("UTF-8")

                    # NOTE: If the chats haven't been refreshed in a while (by the client user),
                    # then it's possible that recv will take more than one chat in at a time. To
                    # ensure all chats are printed on their own line, we'll split the received
                    # message by the header.
                    split_pattern = b"\x02\x06\x03"
                    received_messsage = received_messsage[3:].decode("UTF-8")
                    messages = received_messsage.split(split_pattern.decode("UTF-8"))

                    for message in messages:
                        self.async_alert(_format_line(message))
                        self.cr_state.last_msg_recvd = message

        def _handle_disconnection():
            """
            Handle disconnection from the server.
            """
            self.async_verror("Connection to server lost")
            self.monitor_running = False
            self.shutdown_event.set()

        while self.monitor_running:
            try:
                # Use select to wait for socket events
                readable, _, _ = select.select([self.ssl_socket], [], [], 1.0)
                if readable:
                    with self.ssl_mutex:
                        self.ssl_socket.setblocking(False)
                        if self.chatting_event.is_set():
                            _handle_chats()
                            continue
                        data_read = self.ssl_socket.read(1)
                        if not data_read:
                            _handle_disconnection()
                        break
            except ssl.SSLWantReadError:
                continue
            except (OSError, ssl.SSLError):
                _handle_disconnection()
                break

    def start_socket_monitor(self):
        """Start the socket monitoring thread."""
        if not self.monitor_thread or not self.monitor_thread.is_alive():
            self.monitor_running = True
            self.monitor_thread = threading.Thread(target=self._monitor_socket)
            self.monitor_thread.daemon = (
                True  # Thread will exit when main program exits
            )
            self.monitor_thread.start()

    # ----------------------------- Connected Commands ----------------------------- #

    @cmd2.with_category(CATEGORY_CONNECTED)
    def do_login(self, args):
        """
        Login to the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if self.cr_state.test_login():
            self.vfeedback("You are already logged in.")
            return False

        username = input("Enter your username: ")
        password = getpass.getpass("Enter your password: ")
        packed_message = _cr_messages.login_req_create(username, password)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ACCOUNT_TYPE
                and subtype == _cr_messages.LOGIN_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.cr_state.login(username)
                self.prompt = colored(f"[{username}]> ", "yellow", attrs=["bold"])
                self.voutput(f"{username} logged in.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ACCOUNT_TYPE,
                    _cr_messages.LOGIN_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"login: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_CONNECTED)
    def do_register(self, args):
        """
        Register a new user on the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        username = input("Enter your username: ")
        password = getpass.getpass("Enter your password: ")
        confirm_password = getpass.getpass("Confirm your password: ")

        if password != confirm_password:
            self.verror("Passwords do not match.")
            return False

        packed_message = _cr_messages.register_req_create(username, password)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ACCOUNT_TYPE
                and subtype == _cr_messages.REGISTER_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"{username} registered.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ACCOUNT_TYPE,
                    _cr_messages.REGISTER_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"register: {error}")
            return True
        return False

    # ----------------------------- Logged Commands ----------------------------- #

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_admin(self, args):
        """
        Give admin priviledges to a specified user.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in admin to give admin priviledges.")
            return False

        if self.cr_state.test_chatting():
            return False

        username = input("Enter the username to give admin priviledges to: ")
        packed_message = _cr_messages.admin_req_create(username)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ACCOUNT_TYPE
                and subtype == _cr_messages.ADMIN_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"Updated {username} to admin.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ACCOUNT_TYPE,
                    _cr_messages.ADMIN_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"admin: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_admin_remove(self, args):
        """
        Removes admin priviledges from a specified user.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in admin to remove admin priviledges.")
            return False

        if self.cr_state.test_chatting():
            return False

        username = input("Enter the username to remove admin priviledges from: ")
        packed_message = _cr_messages.admin_r_req_create(username)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ACCOUNT_TYPE
                and subtype == _cr_messages.ADMIN_REMOVE_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"Updated {username} to not admin.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ACCOUNT_TYPE,
                    _cr_messages.ADMIN_REMOVE_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"admin_remove: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_deluser(self, args):
        """
        Delete a user from the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in admin to delete a user.")
            return False

        if self.cr_state.test_chatting():
            return False

        username = input("Enter the username to delete: ")
        packed_message = _cr_messages.user_delete_req_create(username)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ACCOUNT_TYPE
                and subtype == _cr_messages.DEL_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"{username} deleted.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ACCOUNT_TYPE,
                    _cr_messages.DEL_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"deluser: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_list(self, args):
        """
        List all rooms on the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in user to list rooms.")
            return False

        if self.cr_state.test_chatting():
            return False

        packed_message = _cr_messages.list_req_create()

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = b""
            # NOTE: Set timeout to 0.5 seconds to ensure we don't wait too long for a response.
            # Most responses won't require more than one buffer size but the packets should come
            # at about the same time anyways.
            self.ssl_socket.settimeout(0.5)

            try:
                while True:
                    chunk = self.ssl_socket.recv(
                        _cr_messages.MESG_SIZE + _cr_messages.BUFF_SIZE
                    )

                    if not chunk:
                        break

                    received_messsage += chunk
            except TimeoutError:
                # Return timeout to normal
                self.ssl_socket.settimeout(5)

            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ROOMS_TYPE
                and subtype == _cr_messages.LIST_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                room_list = received_messsage[3:].decode("UTF-8")
                self.voutput(f"Rooms:\n{room_list}")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ROOMS_TYPE,
                    _cr_messages.LIST_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"list: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_addroom(self, args):
        """
        Add a room to the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in user to add a room.")
            return False

        if self.cr_state.test_chatting():
            return False

        room_name = input("Enter the name of the room to add: ")
        packed_message = _cr_messages.room_req_create(room_name)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ROOMS_TYPE
                and subtype == _cr_messages.CREATE_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"{room_name} added.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ROOMS_TYPE,
                    _cr_messages.CREATE_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"addroom: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_delroom(self, args):
        """
        Delete a room from the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in user to delete a room.")
            return False

        if self.cr_state.test_chatting():
            return False

        room_name = input("Enter the name of the room to delete: ")
        packed_message = _cr_messages.room_del_req_create(room_name)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ROOMS_TYPE
                and subtype == _cr_messages.DEL_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput(f"{room_name} deleted.")
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ROOMS_TYPE,
                    _cr_messages.DEL_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"delroom: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_join(self, args):
        """
        Join a room on the chat room server.
        """

        def _update_history_format(chat_history: str):
            """
            Update the chat history format to be more readable.
            """

            def _format_line(message: str):
                username, chat_text = message.split(">", 1)
                prompt_str = f"[{username}@{self.cr_state.room}]>"
                colored_prompt = colored(prompt_str, "cyan", attrs=["bold"])
                return f"{colored_prompt}{chat_text}"

            try:
                formatted_history = ""
                for line in chat_history.split("\n"):
                    formatted_history += _format_line(line) + "\n"
            except ValueError:
                pass

            return formatted_history.rstrip("\n")

        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in user to join a room.")
            return False

        if self.cr_state.test_chatting():
            return False

        room_name = input("Enter the name of the room to join: ")
        packed_message = _cr_messages.join_req_create(room_name)

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = b""
            # NOTE: Set timeout to 0.5 seconds to ensure we don't wait too long for a response.
            # Most responses won't require more than one buffer size but the packets should come
            # at about the same time anyways.
            self.ssl_socket.settimeout(0.5)

            try:
                while True:
                    chunk = self.ssl_socket.recv(
                        _cr_messages.MESG_SIZE + _cr_messages.BUFF_SIZE
                    )

                    if not chunk:
                        break

                    received_messsage += chunk
            except TimeoutError:
                # Return timeout to normal
                self.ssl_socket.settimeout(5)

            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.ROOMS_TYPE
                and subtype == _cr_messages.JOIN_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                chat_history = received_messsage[3:].decode("UTF-8")
                self.cr_state.enter_room(room_name, chat_history)
                self.voutput(f"Joined {room_name}.")
                print(
                    colored(
                        _update_history_format(chat_history), "magenta", attrs=["bold"]
                    )
                )
                self.prompt = colored(
                    f"[{self.cr_state.username}@{room_name}]> ",
                    "magenta",
                    attrs=["bold"],
                )
                self.chatting_event.set()
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.ROOMS_TYPE,
                    _cr_messages.JOIN_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"join: {error}")
            return True
        return False

    @cmd2.with_category(CATEGORY_AUTHENTICATED)
    def do_logout(self, args):
        """
        Logout from the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_login():
            self.verror("You must be a logged in user to logout.")
            return False

        if self.cr_state.test_chatting():
            return False

        packed_message = _cr_messages.logout_req_create()
        try:
            self.ssl_socket.sendall(packed_message)
            self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            self.cr_state.logout()
            self.prompt = colored("[NOT LOGGED IN]> ", "yellow", attrs=["bold"])
        except OSError as error:
            self.verror(f"logout: {error}")
            return True
        return False

    # ----------------------------- Chatting State ----------------------------- #

    @cmd2.with_category(CATEGORY_CHATTING)
    def do_leave(self, args):
        """
        Leave a room on the chat room server.
        """
        if self.shutdown_event.is_set():
            self.verror("Connection to server lost. Shutting down...")
            return True

        if not self.cr_state.test_chatting():
            self.verror("You must be in a room to leave.")
            return False

        packed_message = _cr_messages.leave_req_create()

        try:
            self.ssl_socket.sendall(packed_message)
            received_messsage = self.ssl_socket.recv(_cr_messages.MESG_SIZE)
            packet_type, subtype, opcode, reason_code = _cr_messages.cr_recv_unpack(
                received_messsage, self
            )

            if (
                packet_type == _cr_messages.CHAT_TYPE
                and subtype == _cr_messages.LEAVE_STYPE
                and opcode == _cr_messages.ACKNOWLEDGE
            ):
                self.voutput("Left the room.")
                self.prompt = colored(
                    f"[{self.cr_state.username}]> ", "yellow", attrs=["bold"]
                )
                self.cr_state.leave_room()
                self.chatting_event.clear()
            else:
                _cr_utility.cr_handle_not_acks(
                    (packet_type, subtype, opcode, reason_code),
                    _cr_messages.CHAT_TYPE,
                    _cr_messages.LEAVE_STYPE,
                    self.cr_state,
                    self,
                )
        except OSError as error:
            self.verror(f"leave: {error}")
            return True
        return False

    def default(self, statement):
        """
        Send a message to the current room.
        """
        if not self.cr_state.test_chatting():
            self.verror(
                f"{statement.command} is not a recognized command, alias, or macro. "
            )
            return False

        packed_message = _cr_messages.chat_req_create(statement.raw)
        try:
            self.ssl_socket.sendall(packed_message)
        except OSError as error:
            self.verror(f"send: {error}")
            return True
        return False

    # ----------------------------- postloop ----------------------------- #

    def stop_socket_monitor(self):
        """Stop the socket monitoring thread."""
        self.monitor_running = False
        if self.monitor_thread and self.monitor_thread.is_alive():
            self.monitor_thread.join(timeout=1)

    def _pre_cmd_lock(
        self, data: cmd2.plugin.PrecommandData
    ) -> cmd2.plugin.PrecommandData:
        """
        Pre-command function to lock mutex.
        """
        self.ssl_mutex.acquire()
        self.ssl_socket.setblocking(True)
        return data

    def _post_cmd_release(
        self, data: cmd2.plugin.PostcommandData
    ) -> cmd2.plugin.PostcommandData:
        """
        Post-command function to release the mutex.
        """
        self.ssl_mutex.release()
        return data

    def postloop(self):
        """
        Quits the server connections and shuts down the client.
        """
        self.stop_socket_monitor()
        self.ssl_socket.settimeout(5)
        packed_message = _cr_messages.quit_req_create()
        if self.ssl_socket:
            try:
                self.ssl_socket.sendall(packed_message)
                self.ssl_socket.recv(_cr_messages.MESG_SIZE)
                if self.cr_state.test_chatting():
                    self.cr_state.leave_room()
                if self.cr_state.test_login():
                    self.cr_state.logout()
                if self.ssl_socket:
                    self.ssl_socket.close()
                if self.server_socket:
                    self.server_socket.close()
            except OSError as error:
                self.verror(f"postloop error: {error}")


def main():
    """
    Main function to run the Vardorvis command line interface
    """
    try:
        cr_client = ChatRoomClient(hostname="192.168.56.101", port=1234)
    except (OSError, ssl.SSLError):
        print("Chat Room Client Initialization Error")
        return
    cr_client.cmdloop()


if __name__ == "__main__":
    main()
