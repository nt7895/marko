### Build/test container ###
# Define builder stage
FROM marko:base as builder

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Create data directory for database initialization
RUN mkdir -p /usr/src/project/build/data

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on_failure

# Initialize database with test data
RUN ./bin/db_init --reset --seed


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble as deploy

# Copy the compiled server binary from the builder stage into the deployment image
COPY --from=builder /usr/src/project/build/bin/server .

# Ensure that the configuration file is present in the repository in the given path.
COPY --from=builder /usr/src/project/my_config_prod my_config

# Static Handler test files
COPY --from=builder /usr/src/project/usr .
COPY --from=builder /usr/src/project/var .

# TextView Handler test files
COPY --from=builder /usr/src/project/test_pdf .

# Expose some port(s)
EXPOSE 80

# Set the startup command to run the server binary
ENTRYPOINT ["./server"]

# Pass the default configuration file path as an argument to the server
CMD ["my_config"]