# Generic Makefile for project automation
# Customize these targets based on your technology stack

.PHONY: help setup run test lint build clean install

# Default target shows help
.DEFAULT_GOAL := help

help: ## Show this help message
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}'

setup: ## Install dependencies and pre-commit hooks
	@echo "Setting up project..."
	@echo "TODO: Add your setup commands here"
	@echo "Examples:"
	@echo "  - Python: pip install -r requirements.txt && pre-commit install"
	@echo "  - Node.js: npm install && npm run prepare"
	@echo "  - Go: go mod download"
	@echo "  - Java: mvn install"

run: ## Run the application
	@echo "Running application..."
	@echo "TODO: Add your run command here"
	@echo "Examples:"
	@echo "  - Python: python src/main.py"
	@echo "  - Node.js: node src/index.js"
	@echo "  - Go: go run src/main.go"
	@echo "  - Java: java -jar target/app.jar"

test: ## Run tests
	@echo "Running tests..."
	@echo "TODO: Add your test command here"
	@echo "Examples:"
	@echo "  - Python: pytest tests/"
	@echo "  - Node.js: npm test"
	@echo "  - Go: go test ./..."
	@echo "  - Java: mvn test"

lint: ## Run code linter
	@echo "Running linter..."
	@echo "TODO: Add your linting command here"
	@echo "Examples:"
	@echo "  - Python: pylint src/ or flake8 src/"
	@echo "  - Node.js: npm run lint or eslint src/"
	@echo "  - Go: golangci-lint run"
	@echo "  - Java: mvn checkstyle:check"

build: ## Build the project
	@echo "Building project..."
	@echo "TODO: Add your build command here"
	@echo "Examples:"
	@echo "  - Python: pip install -e ."
	@echo "  - Node.js: npm run build"
	@echo "  - Go: go build -o bin/app"
	@echo "  - Java: mvn package"

clean: ## Clean build artifacts
	@echo "Cleaning build artifacts..."
	@echo "TODO: Add cleanup commands here"
	@echo "Examples:"
	@echo "  - rm -rf build/ dist/ *.egg-info __pycache__"
	@echo "  - rm -rf node_modules/.cache"
	@echo "  - go clean"

install: ## Install dependencies
	@echo "Installing dependencies..."
	@echo "TODO: Add your install command here"
	@echo "Examples:"
	@echo "  - Python: pip install -r requirements.txt"
	@echo "  - Node.js: npm install"
	@echo "  - Go: go mod download"
	@echo "  - Java: mvn install"
