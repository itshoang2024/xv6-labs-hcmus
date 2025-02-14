#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLINE 512 // Gioi han do dai dau vao
#define MAXARGS MAXARG // Max tham so

// Stdin
int read_command(char *buffer, int buffer_size) {
    memset(buffer, 0, buffer_size); 
    int length = read(0, buffer, buffer_size - 1); // stdin
    if (length <= 0) return -1; 
    buffer[length] = 0;
    return 0;
}

char whitespace_chars[] = " \t\r\n\v"; // cac ki tu bo qua

// Tach token
int extract_token(char **current_pos, char *end_pos, char **token_start, char **token_end) {
    char *pos = *current_pos;

    // Bo khoang trang
    while (pos < end_pos && strchr(whitespace_chars, *pos)) pos++;
    if (token_start) *token_start = pos; // Luu vi tri bat dau token

    int token_type = *pos; // Ki tu dau tien cua token
    if (*pos) {  
        token_type = 'a'; // Neu co token hop le, dat thanh ki tu a (sau nay co the phan loai)
        while (pos < end_pos && !strchr(whitespace_chars, *pos)) pos++; // Vi tri ket thuc
    }

    if (token_end) *token_end = pos; // Vi tri ket thuc cua toekn
    if (pos < end_pos) *pos++ = 0; // Tach token

    // Bo khoang trang
    while (pos < end_pos && strchr(whitespace_chars, *pos)) pos++;
    *current_pos = pos; // cap nhat pos tiep theo
    return token_type;
}

int main(int argc, char *argv[]) {
    char *command_args[MAXARGS]; // mang chua cac tham so
    // Luu cac doi so tu dong lenh
    for (int i = 1; i < argc; i++) {
        command_args[i - 1] = argv[i];
    }

    static char input_buffer[MAXLINE];
    char *token_start, *token_end; // con tro xac dinh vi tri token

    // Doc lenh va xu li
    while (read_command(input_buffer, sizeof(input_buffer)) >= 0) {
        int arg_index = argc - 1; 
        char *current_pos = input_buffer;
        char *end_pos = current_pos + strlen(current_pos);

        // Lap qua tung token
        while (extract_token(&current_pos, end_pos, &token_start, &token_end) != 0) {
            *token_end = 0;
            command_args[arg_index] = token_start; // Luu token
            command_args[arg_index + 1] = 0;

            // Tien trinh con de thuc hien lenh
            int pid = fork();
            if (pid == 0) {  
                exec(command_args[0], command_args);
                fprintf(2, "xargs: exec failed for %s\n", command_args[0]); // Bao loi
                exit(1);
            } else {  
                wait(0); // Tien trinh cha cho tien trinh con
            }
        }
    }

    exit(0);
}
