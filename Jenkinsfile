pipeline {
agent any
stages {
stage('Checkout') { steps { checkout scm } }
stage('Build') { steps { sh 'make' } }
stage('Static Analysis') { steps { sh 'cppcheck --enable=all altitude_*.c || true' } }
stage('Unit Tests') {
steps {
sh '''
# Start the receiver process in the background for the unit tests on port 5000
./altitude_receiver 5000 received_ut.csv > receiver_ut.log 2>&1 &
echo $! > receiver_ut.pid
# Wait briefly for the receiver to start listening
    sleep 1

    # Run the unit test script, which should now connect to the running receiver
    ./tests/run_unit_tests.sh || true

    # Clean up: Kill the background receiver process
    kill $(cat receiver_ut.pid) || true
    '''
  }
}
stage('Integration Test') {
  steps {
    sh '''
    ./altitude_receiver 5000 received_ci.csv > server.log 2>&1 & echo $! > server.pid
    sleep 1
    ./altitude_streamer 127.0.0.1 5000 5 ci_out.csv 2
    kill $(cat server.pid) || true
    grep "Total samples received: 5" server.log
    '''
  }
}
stage('Docker Build') { steps { sh 'docker build -t myorg/altitude-stream:latest .' } }
}
}

