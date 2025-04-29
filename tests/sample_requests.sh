#!/bin/bash

# 서버를 백그라운드로 실행
./bin/server config/test.conf &
SERVER_PID=$!

sleep 2

# 3번 요청 보내기
curl -s http://localhost:8080
curl -s http://localhost:8080
curl -s http://localhost:8080

sleep 1

# 서버 종료
kill $SERVER_PID
