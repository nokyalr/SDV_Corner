#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define LED_COMMAND_PORT 5055
#define COMMAND_SIZE 32
#define ACK_SIZE 32
#define MAX_RETRIES 3
#define ACK_TIMEOUT_MS 500

static void print_usage(const char *program_name)
{
    printf("Usage: %s <board-ip> <ON|OFF|TOGGLE>\n", program_name);
    printf("Example: %s 10.252.62.53 ON\n", program_name);
}

int main(int argc, char *argv[])
{
    WSADATA wsa_data;
    SOCKET socket_handle;
    struct sockaddr_in board_address;
    struct timeval timeout;
    char command[COMMAND_SIZE];
    char acknowledgement[ACK_SIZE];
    int result;
    int attempt;
    int max_attempts;

    if (argc != 3 ||
        (strcmp(argv[2], "ON") != 0 &&
         strcmp(argv[2], "OFF") != 0 &&
         strcmp(argv[2], "TOGGLE") != 0))
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0)
    {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return EXIT_FAILURE;
    }

    socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_handle == INVALID_SOCKET)
    {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return EXIT_FAILURE;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = ACK_TIMEOUT_MS * 1000;
    if (setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO,
                   (const char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        fprintf(stderr, "setsockopt failed: %d\n", WSAGetLastError());
        closesocket(socket_handle);
        WSACleanup();
        return EXIT_FAILURE;
    }

    result = snprintf(command, sizeof(command), "PB0 %s", argv[2]);
    if (result < 0 || (size_t)result >= sizeof(command))
    {
        fprintf(stderr, "Command is too long.\n");
        closesocket(socket_handle);
        WSACleanup();
        return EXIT_FAILURE;
    }

    max_attempts = strcmp(argv[2], "TOGGLE") == 0 ? 1 : MAX_RETRIES;

    memset(&board_address, 0, sizeof(board_address));
    board_address.sin_family = AF_INET;
    board_address.sin_port = htons(LED_COMMAND_PORT);
    board_address.sin_addr.s_addr = inet_addr(argv[1]);

    if (board_address.sin_addr.s_addr == INADDR_NONE)
    {
        fprintf(stderr, "Invalid board IP address: %s\n", argv[1]);
        closesocket(socket_handle);
        WSACleanup();
        return EXIT_FAILURE;
    }

    for (attempt = 1; attempt <= max_attempts; ++attempt)
    {
        result = sendto(socket_handle,
                        command,
                        (int)strlen(command),
                        0,
                        (const struct sockaddr *)&board_address,
                        sizeof(board_address));
        if (result == SOCKET_ERROR)
        {
            fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
            closesocket(socket_handle);
            WSACleanup();
            return EXIT_FAILURE;
        }

        result = recv(socket_handle, acknowledgement, sizeof(acknowledgement) - 1, 0);
        if (result > 0)
        {
            acknowledgement[result] = '\0';
            if (strcmp(acknowledgement, "OK") == 0)
            {
                printf("Board accepted '%s' on attempt %d.\n", command, attempt);
                closesocket(socket_handle);
                WSACleanup();
                return EXIT_SUCCESS;
            }

            fprintf(stderr, "Board rejected command: %s\n", acknowledgement);
            closesocket(socket_handle);
            WSACleanup();
            return EXIT_FAILURE;
        }
    }

    fprintf(stderr, "No ACK from board after %d attempt(s).\n", max_attempts);

    closesocket(socket_handle);
    WSACleanup();
    return EXIT_FAILURE;
}
