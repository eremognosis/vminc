int main(int argc, char **argv) {
    // Open input file/directory
    // Initialize CodeWriter (write bootstrap code if needed)

    while (has_more_commands()) {
        advance();
        CommandType type = get_command_type();

        switch(type) {
            case C_ARITHMETIC:
                write_arithmetic(out_file, get_arg1());
                break;
            case C_PUSH:
            case C_POP:
                write_push_pop(out_file, type, get_arg1(), get_arg2());
                break;
            // ... handle other types
        }
    }

    return 0;
}