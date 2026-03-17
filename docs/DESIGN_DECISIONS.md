# Architecture Decision Records (ADR)

This document contains Architecture Decision Records for this project. Each ADR captures an important architectural decision along with its context and consequences.

## What is an ADR?

An Architecture Decision Record (ADR) is a document that captures an important architectural decision made along with its context and consequences. ADRs help teams understand:
- Why certain decisions were made
- What alternatives were considered
- What the implications are for future work

## ADR Format

Each decision should follow this structure:

```markdown
## ADR-###: [Decision Title]

**Status:** [Proposed | Accepted | Deprecated | Superseded]
**Date:** YYYY-MM-DD
**Deciders:** [Names or roles]
**Context:** [What is the issue we're trying to solve?]

### Decision
[What is the change we're proposing or have agreed to implement?]

### Alternatives Considered
1. **Alternative 1**: Description
   - Pros: ...
   - Cons: ...
2. **Alternative 2**: Description
   - Pros: ...
   - Cons: ...

### Consequences
- **Positive:** [Benefits of this decision]
- **Negative:** [Drawbacks or technical debt introduced]
- **Neutral:** [Other implications]

### Implementation Notes
[Any specific details about how this decision should be implemented]
```

---

## ADR-001: Use Make for Build Automation

**Status:** Accepted
**Date:** 2026-03-17
**Context:** Need a build automation tool that works across multiple programming languages for this template repository.

### Decision
Use GNU Make as the primary build automation tool for this project template.

### Alternatives Considered

1. **Language-Specific Tools (npm, gradle, cargo, etc.)**
   - Pros: More powerful features specific to each ecosystem
   - Cons: Not portable across languages, requires users to know multiple tools

2. **Task Runners (Task, Just)**
   - Pros: Modern syntax, cross-platform
   - Cons: Not universally installed, additional dependency

3. **Shell Scripts**
   - Pros: No dependencies, maximum flexibility
   - Cons: No built-in dependency management, harder to maintain

### Consequences

- **Positive:**
  - Make is universally available on Unix-like systems
  - Provides a consistent interface regardless of project language
  - Well-understood tool with decades of stability
  - Simple `.PHONY` targets work well for common tasks

- **Negative:**
  - Windows users need to install Make separately (or use WSL)
  - Make syntax can be confusing (tabs vs spaces)
  - Limited compared to modern build tools

- **Neutral:**
  - Projects can layer language-specific tools on top if needed
  - Most developers are familiar enough with Make basics

### Implementation Notes
- Use `.PHONY` targets to avoid conflicts with files
- Include helpful comments explaining what each target does
- Provide a `help` target that lists all available commands
- Keep Makefile simple and language-agnostic

---

## ADR-002: Multi-Stage Docker Build

**Status:** Accepted
**Date:** 2026-03-17
**Context:** Need to demonstrate Docker best practices while keeping production images small and secure.

### Decision
Implement a multi-stage Dockerfile that separates build-time and runtime dependencies.

### Alternatives Considered

1. **Single-Stage Build**
   - Pros: Simpler to understand and maintain
   - Cons: Large image sizes including build tools in production

2. **No Containerization**
   - Pros: Simpler deployment, no Docker knowledge needed
   - Cons: "Works on my machine" problems, less professional

3. **Docker Compose with Multiple Services**
   - Pros: Better separation of concerns
   - Cons: Overkill for a simple template

### Consequences

- **Positive:**
  - Production images are smaller (excludes build tools)
  - Better security (fewer tools means smaller attack surface)
  - Demonstrates understanding of Docker best practices
  - Shows optimization thinking

- **Negative:**
  - Slightly more complex Dockerfile
  - Longer initial build time (though caching helps)
  - Need to understand multi-stage builds to customize

- **Neutral:**
  - Most modern projects use multi-stage builds
  - Well-documented pattern with good examples

### Implementation Notes
- Use `builder` stage for compilation and dependency installation
- Use `production` stage with minimal runtime dependencies
- Run as non-root user for security
- Include health check examples (commented out)
- Provide clear TODOs for language-specific customization

---

## ADR-003: GitHub Actions for CI/CD

**Status:** Accepted
**Date:** 2026-03-17
**Context:** Need CI/CD that's easy to set up, free for open source, and demonstrates industry-standard practices.

### Decision
Use GitHub Actions with separate jobs for linting, testing, building, and security scanning.

### Alternatives Considered

1. **Jenkins**
   - Pros: Highly customizable, self-hosted
   - Cons: Complex setup, not beginner-friendly, hosting costs

2. **GitLab CI**
   - Pros: Good features, integrated with GitLab
   - Cons: Requires GitLab, different syntax

3. **CircleCI / Travis CI**
   - Pros: Good performance, nice UI
   - Cons: Limited free tier, external service

### Consequences

- **Positive:**
  - Zero configuration for GitHub repositories
  - Free for public repositories
  - Native integration with GitHub features
  - Large marketplace of actions
  - Shows understanding of CI/CD principles

- **Negative:**
  - Vendor lock-in to GitHub
  - YAML syntax can be verbose
  - Debugging failed workflows can be tricky

- **Neutral:**
  - Most widely used CI/CD for GitHub projects
  - Easy to migrate to other CI/CD if needed

### Implementation Notes
- Separate jobs for different concerns (lint, test, build, security)
- Use `needs` to create job dependencies
- Include commented examples for coverage reporting
- Gracefully handle projects without tests yet
- Provide clear extension points for customization

---

## Your ADRs

Add your project-specific architectural decisions below:

## ADR-XXX: [Your Decision Title]

**Status:** [Status]
**Date:** YYYY-MM-DD
**Context:** [Description of the problem or need]

### Decision
[What you decided to do]

### Alternatives Considered
[What else you looked at]

### Consequences
[What this means for the project]

### Implementation Notes
[How to implement this decision]
