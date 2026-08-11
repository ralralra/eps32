# 앱 제작 프롬프트 (AI Studio용)

아래 프롬프트를 통째로 복사해서 AI Studio에 붙여넣으면 됩니다.
붙여넣기 전에 **`{{중계서버URL}}` 두 글자만** 실제 배포 URL로 바꾸세요
(중계서버 배포 방법: [`../apps_script/README.md`](../apps_script/README.md)).

---

## 복사할 프롬프트

```
"스마트 출석체크" 웹앱을 만들어줘. 학교에서 선생님이 사용하는 앱이고,
ESP32 RFID 리더기(학생증 태그)와 연동되는 출결 관리 시스템이야.

## 기본 설정

앱 코드 상단에 상수로 정의:
- RELAY_URL = "{{중계서버URL}}"   (Google Apps Script 웹앱, 통신 규칙은 아래 참고)
- DEVICE_ID = "DEV001"           (리더기 이름, 화면에는 "DEV001 (1-3반 입구)"로 표시)

## 디자인

- 밝은 화이트 배경 + 에메랄드/그린 계열(#10b981 톤) 포인트 컬러
- 모바일 우선 반응형, 카드형 모달 UI
- 상단에 방패 아이콘 + "스마트 출석체크" 타이틀

## 화면 구성

### 1. 메인 화면 (탭 3개: 출석체크 / 학생 관리 / 출결 기록)

### 2. [출석체크] 탭
- 교시 선택 (1~7교시, RELAY_URL?action=periods 로 교시별 지각기준 표시)
- 대상 학생 선택 (RELAY_URL?action=students 로 등록된 학생 목록에서 선택,
  "전체(아무나 태그)" 옵션도 제공)
- [출석체크 시작] 버튼 → 모달로 진행:

  (a) 시작 확인 모달: 대상 학생 이름(학년 반), 연동 리더기 "DEV001 (1-3반 입구)" 표시,
      [출석체크 시작] 버튼
  (b) 버튼을 누르면:
      GET RELAY_URL?action=start&mode=attend&device=DEV001&period=교시&name=학생이름&timeout=30
      → 대기 모달로 전환: "출석체크가 시작되었습니다", "장치의 불빛이 깜빡이면 학생증을
      태그해주세요", 30초 카운트다운 진행바, "ESP32 장치 수신 대기중" 배지
  (c) 대기 중 1초마다: GET RELAY_URL?action=status&device=DEV001
      - state === "done" → 결과 모달로 전환
      - state === "none" → "시간 초과" 모달 (다시 시도 버튼)
  (d) 결과 모달: result 값으로 표시
      - result.ok === true → 체크 아이콘, "OO 학생의 N교시 출석이 확인되었습니다",
        인증 시각(result.time), 최종 출결 상태(result.status: "출석"은 초록, "지각"은
        노란 "⚠ 지각 처리" 배지), [확인] 버튼
      - result.reason === "unknown" → "등록되지 않은 카드입니다" (UID 표시)
      - result.reason === "mismatch" → "다른 학생(OO)의 카드입니다"
      - result.reason === "already" → "이미 출석 처리된 학생입니다 (기존 인증: 시각)"
  (e) 모달의 X(닫기) 버튼 → GET RELAY_URL?action=cancel&device=DEV001 호출 후 닫기
  (f) 대기 모달 하단에 개발용 버튼: "[테스트] ESP32 RFID 태그 시뮬레이션"
      → GET RELAY_URL?action=tag&device=DEV001&uid=TEST0001
      (실제 장치와 똑같은 경로로 테스트하는 용도)

### 3. [학생 관리] 탭
- 등록된 학생 목록 표시: GET RELAY_URL?action=students
  (이름, 학년, 반, 번호, 카드 UID, 등록일시)
- [+ 학생증 등록] 버튼 → 등록 모달:
  (a) 입력 폼: 이름(필수), 학년, 반, 번호
  (b) [등록 시작] 버튼:
      GET RELAY_URL?action=start&mode=register&device=DEV001&name=이름&grade=학년&klass=반&number=번호&timeout=30
      → 대기 모달 (출석체크와 같은 30초 카운트다운, "새 학생증을 리더기에 태그해주세요")
  (c) 1초마다 status 폴링 (출석체크와 동일):
      - result.ok === true → "OO 학생의 학생증이 등록되었습니다" (UID 표시)
      - result.reason === "dup" → "이미 OO 학생으로 등록된 카드입니다"
  (d) 여기에도 시뮬레이션 버튼 제공 (uid=TEST0001)

### 4. [출결 기록] 탭
- 날짜 선택 (기본 오늘) + 교시 필터 (전체/1~7교시)
- GET RELAY_URL?action=records&date=yyyy-MM-dd&period=N
- 표로 표시: 시각 | 교시 | 이름 | 상태(출석=초록 배지, 지각=노란 배지) | 리더기
- 상단에 요약: 오늘 출석 N명 · 지각 N명

## 통신 규칙 (중계서버 API)

전부 GET + URL 파라미터 방식이야. fetch에 커스텀 헤더를 절대 붙이지 마
(Content-Type: application/json 등을 붙이면 CORS preflight 때문에 실패해).
응답은 JSON 문자열이니 res.text() 후 JSON.parse 해줘.

- ?action=start&mode=attend|register&device=&period=&name=&grade=&klass=&number=&lateAfter=&timeout=
  → {ok:true, timeout:30, lateAfter:"09:10"}   (세션 생성. 리더기 1대당 1개, timeout 후 자동 만료)
- ?action=status&device=  → {state:"none"|"waiting"|"done", mode, result}
  result 성공: {ok:true, name, status:"출석"|"지각", time:"HH:mm:ss"}  (등록은 {ok:true, name, uid})
  result 실패: {ok:false, reason:"unknown"|"mismatch"|"dup"|"already", name?, time?, uid?}
- ?action=cancel&device=  → {ok:true}
- ?action=students        → {ok:true, students:[{uid,name,grade,klass,number,registeredAt}]}
- ?action=records&date=&period= → {ok:true, date, records:[{date,time,period,uid,name,status,device}]}
- ?action=periods         → {ok:true, periods:[{period,start,lateAfter}]}
- ?action=tag&device=&uid= → 텍스트 응답 (시뮬레이션 버튼용. 실제로는 ESP32가 호출)

## 동작 세부

- 지각 판정은 서버가 한다 (교시설정 시트의 지각기준 시각 기준). 앱은 결과만 표시.
- 출석 세션에서 대상 학생을 "전체"로 하면 start의 name 파라미터를 생략해 —
  그러면 등록된 학생 누구든 태그해서 출석할 수 있어.
- 네트워크 오류 시 사용자에게 친절한 한국어 메시지를 보여주고 재시도 버튼 제공.
```

---

## 이미 만든 앱을 수정하는 경우

앱을 처음부터 다시 만들지 않고 기존 앱(시뮬레이션 버튼만 있는 버전)에
실제 연동만 붙이려면 → [`app_integration.md`](app_integration.md) 의 짧은 수정 프롬프트를 쓰세요.

## 연동 확인 순서

1. 중계서버 배포 후, 브라우저에서 `URL?action=periods` → 교시 JSON이 나오는지
2. 앱에서 [학생증 등록] → ESP32 LED 빠르게 깜빡 → 학생증 태그 → 등록 완료 확인
3. 시트의 **학생명단**에 UID가 들어갔는지 확인
4. 앱에서 [출석체크 시작] → LED 천천히 깜빡 → 태그 → "출석 확인" + **출석기록** 시트 확인
