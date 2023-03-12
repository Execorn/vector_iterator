#ifndef X17_PRINTF_VARIADIC
#define X17_PRINTF_VARIADIC

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <ostream>

namespace X17 {

void m_printf(const char* format) {
    while (*format != '\0') {
        for (; *format != '%' && *format != '\0'; ++format) {
            std::putchar(*format);
        }    

        // if we got a %, it's either "...%%..." or user error
        if (*format == '%') {
            // in case we got "...%%..." in string
            if (*(format + 1) == '%') {
                std::putchar(*(format++));
            } else {
                throw std::invalid_argument("not enough arguments");
            }
        } 
    }   
} 

template<typename T, typename... ArgsT>
void m_printf(const char* format, T next_arg, ArgsT... arguments) {
    // print all symbols before format specifier
    for(; *format != '\0' && *format != '%'; ++format) {
        std::putchar(*format);
    }    

    // if we hit end of string, just leave the function
    if (*format == '\0') {
        return;
    }

    // std::cout is overloaded for every type we need 
    std::cout << next_arg;
    // pass args without next_arg, so the first arg
    // in arguments... becomes next_arg 
    m_printf(format + 2, arguments...);
}   

}

#endif // !X17_PRINTF_VARIADIC