"""Tests the Chat Room Server/client:
We take a unique approach to testing the server by calling the client via pytest,
and verifying the server's responses. The order of tests in the file is important,
as many tests are dependent on the state of the server.

WARNING: The monitor thread will be running.
"""

# pytest fixture casuses redefined-outer-name
# pylint: disable=redefined-outer-name

import io
from unittest.mock import patch

import pytest

from chatroomclient.cr_client import ChatRoomClient


@pytest.fixture
def client():
    """
    Fixture to create a ChatRoomClient instance.
    """
    chat_client = ChatRoomClient(hostname="192.168.56.101", port=1234)

    chat_client.ssl_socket.settimeout(5)
    with chat_client.ssl_mutex:

        yield chat_client

    chat_client.postloop()


@pytest.fixture
def adminclient():
    """
    Fixture to create an admin client.
    """
    admin_client = ChatRoomClient(hostname="192.168.56.101", port=1234)

    admin_client.ssl_socket.settimeout(5)
    with admin_client.ssl_mutex:

        with patch("builtins.input", return_value="admin"):
            with patch("getpass.getpass", return_value="password"):
                admin_client.do_login("login")

        yield admin_client

    admin_client.postloop()


def test_login(client: ChatRoomClient):
    """
    Testing successful ack packet return from server.
    """
    with patch("builtins.input", return_value="admin"):
        with patch("getpass.getpass", return_value="password"):
            assert False is client.do_login("login")
            assert client.cr_state.test_login()


def test_login_fail(client: ChatRoomClient):
    """
    Testing incorrect passwords.
    """
    with patch("builtins.input", return_value="admin"):
        with patch("getpass.getpass", return_value="p"):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()

    with patch("builtins.input", return_value="admin"):
        with patch("getpass.getpass", return_value="passwords"):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()

    with patch("builtins.input", return_value="uknown_user"):
        with patch("getpass.getpass", return_value="password"):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()

    with patch("builtins.input", return_value="uknown_user"):
        with patch("getpass.getpass", return_value=""):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()

    with patch("builtins.input", return_value=""):
        with patch("getpass.getpass", return_value="pass"):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()


def test_register(client: ChatRoomClient):
    """
    Testing register of a user.
    """
    with patch("builtins.input", return_value="newuser"):
        with patch("getpass.getpass", return_value="password"):
            assert False is client.do_login("login")
            assert not client.cr_state.test_login()

    with patch("builtins.input", return_value="newuser"):
        with patch("getpass.getpass", return_value="password"):
            with patch("getpass.getpass", return_value="password"):
                assert False is client.do_register("register")

    with patch("builtins.input", return_value="newuser"):
        with patch("getpass.getpass", return_value="password"):
            assert False is client.do_login("login")
            assert client.cr_state.test_login()


def test_deluser(adminclient: ChatRoomClient):
    """
    Testing deletion of a user.
    """
    with patch("builtins.input", return_value="newuser"):
        assert False is adminclient.do_deluser("deluser")

    assert False is adminclient.do_logout("logout")

    with patch("builtins.input", return_value="newuser"):
        with patch("getpass.getpass", return_value="password"):
            assert False is adminclient.do_login("login")
            assert not adminclient.cr_state.test_login()


def test_addroom(adminclient: ChatRoomClient):
    """
    Testing adding a room by redirecting the client's stdout.
    """
    output_buffer = io.StringIO()
    adminclient.stdout = output_buffer

    adminclient.do_list("list")

    initial_output = output_buffer.getvalue()
    output_buffer.seek(0)
    output_buffer.truncate(0)

    assert "newroom" not in initial_output

    with patch("builtins.input", return_value="newroom"):
        assert False is adminclient.do_addroom("addroom")

    adminclient.do_list("list")

    final_output = output_buffer.getvalue()

    assert "newroom" in final_output


def test_delroom(adminclient: ChatRoomClient):
    """
    Testing deleting a room by redirecting the client's stdout.
    This test is self-contained.
    """
    output_buffer = io.StringIO()
    adminclient.stdout = output_buffer

    def read_and_clear_buffer():
        value = output_buffer.getvalue()
        output_buffer.seek(0)
        output_buffer.truncate(0)
        return value

    with patch("builtins.input", return_value="newroom"):
        adminclient.do_addroom("addroom")
    read_and_clear_buffer()

    adminclient.do_list("list")
    assert "newroom" in read_and_clear_buffer()

    with patch("builtins.input", return_value="newroom"):
        adminclient.do_delroom("delroom")
    read_and_clear_buffer()

    adminclient.do_list("list")
    assert "newroom" not in read_and_clear_buffer()


# NOTE:Due to the nature of threading and a number of input sources, it's difficult to produce unit
# tests for the remaining functions. Instead, they are tested through running the program.

# End of test_me.py file
