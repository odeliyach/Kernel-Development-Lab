# Tests

Place your test files here.

This directory should mirror the structure of your `src/` directory.

## Testing Guidelines

- Write tests for all critical functionality
- Aim for good test coverage (70%+ is a reasonable target)
- Use descriptive test names that explain what is being tested
- Follow the AAA pattern: Arrange, Act, Assert

## Example structures:

### Python (pytest)
```
tests/
├── __init__.py
├── test_main.py
├── test_models.py
└── test_services.py
```

### Node.js (Jest/Mocha)
```
tests/
├── unit/
│   ├── controllers.test.js
│   └── models.test.js
└── integration/
    └── api.test.js
```

### Go
```
tests/
├── handlers_test.go
├── models_test.go
└── integration_test.go
```

### Java (JUnit)
```
tests/
└── java/
    └── com/
        └── yourproject/
            ├── ModelTest.java
            └── ServiceTest.java
```

## Running Tests

Configure the `make test` target in the root Makefile to run your tests.
