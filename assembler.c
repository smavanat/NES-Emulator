#include <stdio.h>

void write_text(char *text, size_t len) {
    FILE *fptr = fopen("test.txt", "w");
    fwrite(text, sizeof(char), len, fptr);
    fclose(fptr);
}

int main(void) {
    write_text("Hello World", 11);

    return 0;
}
