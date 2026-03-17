# Multi-stage Dockerfile - Customize for your specific runtime requirements
# This approach keeps production images small by separating build and run stages

# Build stage - includes build tools and dependencies
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /app

# Install build dependencies
# TODO: Add language-specific build tools and compilers
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copy dependency files first for better layer caching
# TODO: Adjust based on your language
# Examples:
# Python: COPY requirements.txt .
# Node.js: COPY package*.json .
# Go: COPY go.mod go.sum .
# Java: COPY pom.xml .

# Install dependencies
# TODO: Add your language-specific dependency installation
# Examples:
# Python: RUN pip install --no-cache-dir -r requirements.txt
# Node.js: RUN npm ci --only=production
# Go: RUN go mod download
# Java: RUN mvn dependency:go-offline

# Copy application source
COPY . .

# Build the application (if needed)
# TODO: Add your build command
# Examples:
# Python: (usually no build step)
# Node.js: RUN npm run build
# Go: RUN go build -o /app/bin/app -ldflags="-w -s" ./src
# Java: RUN mvn package -DskipTests

# Production stage - minimal runtime image
FROM ubuntu:22.04 AS production

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /app

# Install only runtime dependencies (no build tools)
# TODO: Add language-specific runtime only
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Examples of runtime installations:
# Python: RUN apt-get install -y python3 python3-pip && rm -rf /var/lib/apt/lists/*
# Node.js: RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && apt-get install -y nodejs && rm -rf /var/lib/apt/lists/*
# Go: (binary is self-contained, no runtime needed)
# Java: RUN apt-get install -y openjdk-17-jre-headless && rm -rf /var/lib/apt/lists/*

# Copy built artifacts from builder stage
# TODO: Adjust paths based on your build output
# COPY --from=builder /app/dist ./dist
# COPY --from=builder /app/bin/app ./bin/app
# COPY --from=builder /app/target/*.jar ./app.jar

# For now, copy everything (customize this)
COPY --from=builder /app .

# Create non-root user for security
RUN useradd -m -u 1000 appuser && \
    chown -R appuser:appuser /app
USER appuser

# Expose port (customize as needed)
# TODO: Change to your application's port
EXPOSE 8080

# Health check (optional but recommended for production)
# HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
#   CMD curl -f http://localhost:8080/health || exit 1

# Define default command
# TODO: Update with your application's start command
CMD ["echo", "TODO: Configure your application start command"]
# Examples:
# Python: CMD ["python3", "src/main.py"]
# Node.js: CMD ["node", "dist/index.js"]
# Go: CMD ["./bin/app"]
# Java: CMD ["java", "-jar", "app.jar"]
