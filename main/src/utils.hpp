#pragma once

template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

/**
 * @author iFreilicht
 * @file https://stackoverflow.com/a/26221725
 * @tparam Args The types of the arguments
 * @param format The format string
 * @param args The arguments
 * @return The formatted string
 */
template<typename ...Args>
std::string string_format(const char* format, Args ...args )
{
    int size_s = std::snprintf(nullptr, 0, format, args ... ) + 1;
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format, args ... );
    return { buf.get(), buf.get() + size - 1 };
}
