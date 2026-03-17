# GitHub Copilot Instructions

## Writing Style

Write naturally and conversationally, like a human developer would.

- Never use em dashes (—)
- Keep sentences short and clear
- Avoid AI-like phrasing and corporate jargon
- Write concisely without unnecessary elaboration

## Code Comments

Always add comments to non-trivial code. Focus on explaining WHY something is done, not just WHAT it does.

Highlight:
- Tradeoffs made in the implementation
- Complexity considerations
- Design decisions and rationale

## Style Enforcement

Follow these coding standards consistently:

- **No Magic Numbers**: Always use named constants instead of hardcoded values
  - Bad: `if (retry < 3)`
  - Good: `const MAX_RETRIES = 3; if (retry < MAX_RETRIES)`
- **Consistent Naming**: Follow the project's naming conventions (check existing code)
- **Error Handling**: Always handle errors explicitly, never silently ignore them
- **DRY Principle**: Don't repeat yourself. Extract common patterns into reusable functions
- **Single Responsibility**: Each function should do one thing well

## Interview Prep Mode

After implementing or modifying significant code:

1. **Ask**: "Would you like to update INTERVIEW_NOTES.md with this change?"
2. **Suggest** adding to INTERVIEW_NOTES.md:
   - Trade-off bullet points for architectural decisions
   - Complexity analysis (time/space) for algorithms
   - Edge cases handled
   - Scaling considerations
   - Alternative approaches considered

Example prompt:
```
This implementation uses [approach]. I've added it to the code.

Would you like me to add an entry to INTERVIEW_NOTES.md covering:
- Why we chose [approach] over [alternative]
- Time complexity: O(n)
- Trade-off: [benefit] vs [cost]
```

## Code Quality Standards

Prefer patterns that are:
- **Robust**: Handle edge cases and errors gracefully
- **Maintainable**: Easy for others (or future you) to understand and modify
- **Testable**: Written in a way that makes testing straightforward
- **Clear**: Self-documenting code over clever one-liners

Avoid:
- Clever tricks that sacrifice readability
- Premature optimization
- Over-engineering simple problems
- Leaving TODO comments without creating issues

## Function Documentation

For public functions, always include:
- Brief description of what it does
- Parameters and their types/constraints
- Return value and type
- Any side effects or exceptions
- Example usage (for complex functions)

## Interview-Focused Development

When code is added or modified, automatically:

1. Add interview-focused comments directly in the code that explain:
   - Why this approach was chosen
   - What alternatives were considered
   - Key tradeoffs and limitations
   - Performance or scalability implications

2. Suggest updating INTERVIEW_NOTES.md with:
   - Key decisions made
   - Tradeoffs considered
   - Complexity analysis
   - Potential improvements
   - Realistic interview questions with strong senior-level answers

## General Guidelines

- Prioritize readability and maintainability
- Consider edge cases and error handling
- Think about how the code would scale
- Document assumptions and constraints
- Write code that you would be proud to discuss in an interview
- When suggesting code changes, explain the "why" behind your suggestions
