# Use a clean, stable base image for the build environment
FROM ubuntu:22.04

# Install necessary build tools (like gcc, g++)
RUN apt-get update && apt-get install -y build-essential

# Set the working directory inside the container
WORKDIR /app

# Copy the entire project context (source files, Makefiles, etc.) into the container
COPY . /app

# Run 'make' to build the binaries (altitude_receiver and altitude_streamer)
RUN make -f Makefile.mk

# Expose the port used by the receiver application
EXPOSE 5000

# Set the default command to run the receiver when the container starts
# Note: The port 5000 here should match the one used in the CMD array.
CMD ["./altitude_receiver", "5000", "received.csv"]

