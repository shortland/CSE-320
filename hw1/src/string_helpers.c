int string_length(char *string) {
    int c = 0;

    while (*(string + c) != '\0')
        c++;

    return c;
}