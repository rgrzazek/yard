# Use an official OpenJDK image as the base image
FROM amazoncorretto:25-alpine

# Run as a non-root user — a compromised process inside the container
# shouldn't have root capability by default.
RUN addgroup -S app && adduser -S app -G app

# Set the working directory inside the container
WORKDIR /app

# Copy the built JAR file into the container
COPY --chown=app:app build/libs/yard-0.0.1-SNAPSHOT.jar app.jar

USER app

# Expose the port that the Spring Boot app will run on
EXPOSE 8080

# Actuator's health endpoint is exposed over web by default, no extra config needed.
HEALTHCHECK --interval=30s --timeout=3s --start-period=30s --retries=3 \
  CMD wget -q --spider http://localhost:8080/actuator/health || exit 1

# Run the JAR file when the container starts
ENTRYPOINT ["java", "-jar", "app.jar"]
