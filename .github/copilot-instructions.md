# GitHub Copilot Instructions - Kernel Development Focus

## Writing Style

Write naturally and conversationally, like a human developer would.

- Never use em dashes (—)
- Keep sentences short and clear
- Avoid AI-like phrasing and corporate jargon
- Write concisely without unnecessary elaboration

## Code Comments for Kernel Development

Always add comments to non-trivial code. Focus on explaining WHY something is done, not just WHAT it does.

Highlight:
- Tradeoffs made in the implementation
- Complexity considerations
- Design decisions and rationale
- Kernel-specific constraints (memory allocation, user-space access, locking)

## Kernel Coding Standards

Follow Linux kernel coding style consistently:

- **Use kernel logging**: `printk(KERN_ERR ...)` instead of printf
- **Memory allocation**: Always use `kmalloc(GFP_KERNEL)` and check return values
- **User-space access**: Always use `get_user`/`put_user` or `copy_from_user`/`copy_to_user`
- **Error codes**: Return standard errno values (EINVAL, ENOMEM, EFAULT, etc.)
- **No floating point**: Kernel space doesn't support floating point operations
- **Header guards**: Use `#ifndef MODULE_H` / `#define MODULE_H` / `#endif`

## Style Enforcement

Follow these coding standards consistently:

- **No Magic Numbers**: Always use named constants instead of hardcoded values
  - Bad: `if (len > 128)`
  - Good: `#define MAX_MESSAGE_LEN 128; if (len > MAX_MESSAGE_LEN)`
- **Consistent Naming**: Follow kernel conventions (lowercase with underscores)
  - Types: `struct_name_t` or `struct_name`
  - Functions: `module_function_name()`
  - Constants: `UPPERCASE_WITH_UNDERSCORES`
- **Error Handling**: Always handle errors explicitly, never silently ignore them
  - Check all `kmalloc` returns
  - Check all user-space copy operations
  - Return appropriate errno codes
- **DRY Principle**: Don't repeat yourself. Extract common patterns into reusable functions
- **Single Responsibility**: Each function should do one thing well

## Doxygen Documentation

For all functions in kernel modules, include Doxygen comments:

```c
/**
 * @brief Brief description of function
 *
 * Detailed description explaining what the function does,
 * any important implementation details, and why design
 * decisions were made.
 *
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * @retval VALUE Description of specific return value (e.g., -EINVAL for invalid input)
 */
```

## Interview Prep Mode

After implementing or modifying significant code:

1. **Ask**: "Would you like to update INTERVIEW_NOTES.md with this change?"
2. **Suggest** adding to INTERVIEW_NOTES.md:
   - Trade-off bullet points for architectural decisions
   - Complexity analysis (time/space) for algorithms
   - Edge cases handled
   - Scaling considerations
   - Alternative approaches considered
   - Security considerations (especially for kernel code)

Example prompt:
```
This implementation uses [approach] for [feature]. I've added it to the code.

Would you like me to add an entry to INTERVIEW_NOTES.md covering:
- Why we chose [approach] over [alternative]
- Time complexity: O(n)
- Trade-off: [benefit] vs [cost]
- Security implications: [consideration]
```

## Code Quality Standards

Prefer patterns that are:
- **Robust**: Handle edge cases and errors gracefully
- **Maintainable**: Easy for others (or future you) to understand and modify
- **Testable**: Written in a way that makes testing straightforward
- **Clear**: Self-documenting code over clever one-liners
- **Safe**: Especially for kernel code - validate all inputs, check all returns

Avoid:
- Clever tricks that sacrifice readability
- Premature optimization
- Over-engineering simple problems
- Leaving TODO comments without creating issues
- Unsafe user-space memory access

## Kernel-Specific Guidelines

### Memory Management
- Always check `kmalloc` return values before using the pointer
- Use `GFP_KERNEL` for most allocations (can sleep)
- Use `GFP_ATOMIC` for allocations in interrupt context (cannot sleep)
- Always free allocated memory in cleanup/exit functions
- Avoid memory leaks - track all allocations

### User-Space Interaction
- Never directly dereference user-space pointers
- Always use `get_user`, `put_user`, `copy_from_user`, or `copy_to_user`
- Check return values of user-space copy operations
- Return `-EFAULT` if copy operations fail

### Concurrency and Locking
- Document whether locking is needed and why
- If locks are needed:
  - Use spinlocks for short critical sections
  - Use mutexes for longer critical sections
  - Use `spin_lock_irqsave` to disable interrupts
- Document the locking strategy in comments

### Error Handling
- Return negative errno values on error (e.g., `-EINVAL`, `-ENOMEM`)
- Return 0 or positive values on success
- For read/write: return number of bytes transferred or negative errno
- Always clean up partially allocated resources on error paths

### Module Structure
```c
// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author Name");
MODULE_DESCRIPTION("Brief description");

// Module initialization
static int __init module_init_function(void) {
    // Initialize resources
    // Return 0 on success, negative errno on failure
}

// Module cleanup
static void __exit module_exit_function(void) {
    // Free all resources
    // Clean up all state
}

module_init(module_init_function);
module_exit(module_exit_function);
```

## Function Documentation Example

```c
/**
 * @brief Write a message to the current channel
 *
 * Writes a message to the channel associated with this file descriptor.
 * If censorship is enabled, every 4th character is replaced with '#'.
 * The write operation is atomic - either the entire message is written
 * or none of it is.
 *
 * @param file Pointer to the file structure
 * @param buffer User-space buffer containing the message
 * @param len Length of the message
 * @param offset File offset (unused)
 * @return Number of bytes written on success, negative error code on failure
 * @retval -EINVAL No channel has been set on the file descriptor
 * @retval -EMSGSIZE Message length is 0 or exceeds MAX_MESSAGE_LEN
 * @retval -EFAULT Failed to copy data from user space
 * @retval -ENOMEM Memory allocation failed
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t len, loff_t *offset) {
    // Implementation
}
```

## Interview-Focused Development

When code is added or modified, automatically:

1. Add interview-focused comments directly in the code that explain:
   - Why this approach was chosen
   - What alternatives were considered
   - Key tradeoffs and limitations
   - Performance or scalability implications
   - Security considerations (especially for kernel code)

2. Suggest updating INTERVIEW_NOTES.md with:
   - Key decisions made
   - Tradeoffs considered
   - Complexity analysis
   - Potential improvements
   - Realistic interview questions with strong senior-level answers
   - Kernel-specific topics (memory management, concurrency, security)

## General Guidelines

- Prioritize readability and maintainability
- Consider edge cases and error handling
- Think about how the code would scale
- Document assumptions and constraints
- Write code that you would be proud to discuss in an interview
- When suggesting code changes, explain the "why" behind your suggestions
- For kernel code: prioritize safety and correctness over performance

## Kernel Development Checklist

Before submitting code, ensure:
- [ ] All `kmalloc` calls check return values
- [ ] All user-space access uses proper copy functions
- [ ] All error paths clean up partial allocations
- [ ] Module init/exit properly register/unregister resources
- [ ] Appropriate errno codes returned on errors
- [ ] Doxygen comments on all functions
- [ ] No floating point operations
- [ ] Proper locking documented (if applicable)
- [ ] Academic Integrity Notice present in LICENSE
- [ ] Professional documentation for portfolio use
