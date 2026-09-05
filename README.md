# ft_irc

C++98과 POSIX 소켓으로 구현한 IRC 형식의 채팅 서버입니다. 하나의 프로세스에서 `poll()`로 여러 클라이언트의 입력을 감시하고, 사용자 등록·채널 참여·개인 및 채널 메시지·권한 검사를 처리합니다.

42 Seoul 프로젝트로, **소켓 이벤트 처리와 사용자·채널 상태를 연결하는 과정**을 구현했습니다.

## 코드 구조

| 파일 | 역할 |
| --- | --- |
| [main.cpp](main.cpp) | 실행 인자, SIGINT·SIGPIPE 처리, 서버 루프 |
| [server.cpp](srcs/server.cpp) · [server.hpp](srcs/server.hpp) | TCP 소켓 초기화, 포트·비밀번호, 사용자·채널 저장 |
| [serverHandler.cpp](srcs/serverHandler.cpp) | poll·accept·recv 처리, 명령 분기, 메시지 전달과 권한 검사 |
| [user.cpp](srcs/user.cpp) · [user.hpp](srcs/user.hpp) | 닉네임·사용자명, 등록 상태, 관리자·채널 호스트 상태 |

서버는 `pollfd` 배열로 연결을 감시하고, 파일 디스크립터를 키로 사용자 객체를 찾습니다. 채널은 이름별 사용자 FD 목록으로 관리합니다.

## 빌드와 시작

C++ 컴파일러, Make, POSIX 소켓 환경이 필요합니다. Makefile은 C++98과 AddressSanitizer를 사용합니다.

```bash
make
./ircserv 6667 demo42
```

인자는 `<포트> <접속 비밀번호>` 순서입니다. 포트 범위는 1024–49151이며, 비밀번호·닉네임·사용자명은 영문자와 숫자를 사용합니다.

서버는 모든 IPv4 인터페이스에 바인딩합니다. 학습용 관리자 비밀번호 `admin`이 코드에 고정되어 있으므로 로컬 실습 환경에서 실행합니다. 종료는 `Ctrl+C`, 빌드 정리는 `make fclean`입니다.

## 두 클라이언트로 대화하기

별도 터미널 두 개에서 각각 `nc 127.0.0.1 6667`로 연결합니다. 명령은 대문자로 입력합니다.

**첫 번째 클라이언트**

```text
PASS demo42
NICK alice
USER alice 0 * :Alice
JOIN #demo
```

**두 번째 클라이언트**

```text
PASS demo42
NICK bob
USER bob 0 * :Bob
JOIN #demo
```

두 클라이언트가 참여한 뒤 첫 번째에서 전송합니다.

```text
PRIVMSG #demo :hello
PRIVMSG bob hello
```

두 번째 클라이언트가 받는 메시지입니다.

```text
:alice!alice@127.0.0.1 PRIVMSG #demo :hello
:alice!alice@127.0.0.1 PRIVMSG bob :hello
```

## 명령과 권한

| 명령 | 현재 구현 |
| --- | --- |
| `PASS` → `NICK` → `USER` | 비밀번호 확인 후 닉네임·사용자명을 등록 |
| `NICK <이름>` | 중복 확인 후 닉네임 변경 |
| `JOIN #채널` · `PART #채널` | 채널 생성·참여와 이탈 |
| `PRIVMSG <닉네임 또는 #채널> <메시지>` | 개인 메시지 또는 참여 중인 채널에 전달 |
| `NOTICE` | 메시지 처리 함수를 공유. 개인 대상은 `PRIVMSG` 형식으로 전달 |
| `LIST` | 전체 채널과 자신의 참여 채널 출력 |
| `KICK #채널 닉네임` | 관리자 또는 채널 호스트가 사용자를 퇴장시킴 |
| `HOST #채널 닉네임` | 관리자가 해당 사용자의 호스트 상태를 전환하는 자체 명령 |
| `QUIT` | 참여 채널 정리와 소켓 연결 해제 |

처음 채널을 만든 사용자가 호스트가 됩니다. 일반 호스트는 관리자나 다른 호스트를 추방할 수 없으며, 관리자도 다른 관리자는 추방할 수 없습니다. 관리자 등록에는 `PASS admin`을 사용합니다.

## 메시지 처리

1. 서버의 리스닝 소켓에 새 연결 요청이 들어오면 `accept()`로 클라이언트 소켓을 만들고 사용자를 등록합니다.
2. 사용자 소켓의 입력을 읽고 줄 단위로 명령과 인자를 나눕니다.
3. 등록 전에는 `registrationUser()`, 등록 후에는 `parsingMSG()`가 처리합니다.
4. 명령별 함수가 사용자·채널 상태를 확인하고 응답하거나 다른 소켓으로 전달합니다.

소켓은 non-blocking으로 설정합니다. 현재 수신 처리는 한 번의 `recv()` 결과를 바로 파싱하며, 분할 수신된 명령을 다음 입력과 합치는 버퍼나 부분 송신 재시도 큐는 구현되어 있지 않습니다.

## 개발 기록

[최종 USER 파싱 수정](https://github.com/tjung03/ft_irc/commit/4a530e9dabd45a0b9e2af14cc61b68d7ac002afd)에서는 명령 인자의 첫 번째 값만 사용자명으로 저장하도록 변경했습니다.
