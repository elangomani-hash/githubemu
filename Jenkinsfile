pipeline {
  agent any
  stages {
    stage('Checkout') { steps { checkout scm } }
    stage('Build') { steps { sh 'make' } }
    stage('Static Analysis') { steps { sh 'cppcheck --enable=all altitude_*.c || true' } }
    stage('Unit Tests') { steps { sh './tests/run_unit_tests.sh || true' } }
    stage('Integration Test') {
      steps {
        sh '''
        ./altitude_receiver 6000 received_ci.csv > server.log 2>&1 & echo $! > server.pid
        sleep 1
        ./altitude_streamer 127.0.0.1 6000 5 ci_out.csv 2
        kill $(cat server.pid) || true
        grep "Total samples received: 5" server.log
        '''
      }
    }
    stage('Docker Build') { steps { sh 'docker build -t myorg/altitude-stream:latest .' } }
  }
}

