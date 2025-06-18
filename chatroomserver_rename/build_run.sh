./build.sh
rm -rf rooms
# valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/chat_room
./build/chat_room
