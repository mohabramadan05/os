#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int c;

    printf("User-space process:\n");
    printf("PID: %d\n", getpid());
    printf("Press 'q' + Enter to quit.\n");
    printf("Load the kernel module now to inspect this process\n\n");

    while (1) {
        printf("> ");
        fflush(stdout);
        c = getchar();
        while (getchar() != '\n');
        if (c == 'q' || c == 'Q') {
            printf("Quitting.\n");
            break;
        }
        printf("Still running... (pid=%d)\n", getpid());
    }

    return 0;
}