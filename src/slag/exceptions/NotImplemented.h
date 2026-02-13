#ifndef SLAG_NOTIMPLEMENTED_H
#define SLAG_NOTIMPLEMENTED_H
#include <stdexcept>
#include <source_location>
#include <string>

namespace slag
{
    class NotImplemented: public std::logic_error
    {
    private:

        std::string _text;

    public:
        explicit NotImplemented(const std::source_location& location = std::source_location::current())
            :
            std::logic_error("Not Implemented")
        {
            _text = "Not Implemented";
            _text += " : ";
            _text += location.function_name();
            _text += "(";
            _text += location.file_name();
            _text += "@";
            _text += std::to_string(location.line());
            _text += ")";
        };

        virtual const char *what() const throw()
        {
            return _text.c_str();
        }
    };
} // slag

#endif //SLAG_NOTIMPLEMENTED_H
