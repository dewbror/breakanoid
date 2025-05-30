#ifndef CONFIG_H_
#define CONFIG_H_

// Check for GCC or Clang
#if defined(__GNUC__) || defined(__clang__)
#define FORMAT_ATTR(format_index, first_arg_index) __attribute__((format(printf, format_index, first_arg_index)))
#else
#define FORMAT_ATTR(format_index, first_arg_index) // NOP for MSVC and other compilers
#endif

#endif // CONFIG_H_
