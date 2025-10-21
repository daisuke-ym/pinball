/*
 * Serial Interface for Arduino
 * シリアル通信でCANIDとPAYLOADを受け取ってCANバスに流す通信インターフェース
 */

// バッファサイズの定義
#define MAX_HEX_VALUES 8

// グローバル変数
uint16_t canId = 0;
uint8_t payload[MAX_HEX_VALUES - 1];  // CANIDを除いた分
int payloadLength = 0;

void serialInterface() {
  // シリアルデータが利用可能かチェック
  if (Serial.available()) {
    // 改行文字まで文字列を読み取り
    String inputString = Serial.readStringUntil('\n');
    
    if (inputString.length() > 0) {
      // 前後の空白と改行文字を削除
      inputString.trim();
      
      Serial.print("Received: ");
      Serial.println(inputString);
      
      // データをクリア
      canId = 0;
      payloadLength = 0;
      memset(payload, 0, sizeof(payload));
      
      int valueCount = 0;
      int startIndex = 0;
      
      // スペースで文字列を分割して処理
      while (startIndex < inputString.length() && valueCount < MAX_HEX_VALUES) {
        // スペースをスキップ
        while (startIndex < inputString.length() && 
               inputString.charAt(startIndex) == ' ') {
          startIndex++;
        }
        
        if (startIndex >= inputString.length()) break;
        
        // 次のスペースを見つける
        int endIndex = startIndex;
        while (endIndex < inputString.length() && 
               inputString.charAt(endIndex) != ' ') {
          endIndex++;
        }
        
        // トークンを抽出
        String token = inputString.substring(startIndex, endIndex);
        
        // 16進数の解析を試行
        unsigned long value = parseHexString(token);
        if (value != 0xFFFFFFFF) {  // 解析エラーでない場合
          if (valueCount == 0) {
            // 1つ目はCAN ID (16ビット)
            if (value <= 0xFFFF) {
              canId = (uint16_t)value;
              valueCount++;
            }
            else {
              Serial.print("Error: CAN ID must be 16-bit (0x0000-0xFFFF), got 0x");
              Serial.println(value, HEX);
            }
          }
          else {
            // 2つ目以降はペイロード (8ビット)
            if (value <= 0xFF && payloadLength < (MAX_HEX_VALUES - 1)) {
              payload[payloadLength] = (uint8_t)value;
              payloadLength++;
              valueCount++;
            }
            else if (value > 0xFF) {
              Serial.print("Error: Payload byte ");
              Serial.print(payloadLength + 1);
              Serial.print(" must be 8-bit (0x00-0xFF), got 0x");
              Serial.println(value, HEX);
            }
            else {
              Serial.println("Error: Too many payload bytes");
            }
          }
        }
        else {
          Serial.print("Error: Invalid hex value '");
          Serial.print(token);
          Serial.println("'");
        }
        
        startIndex = endIndex;
      }
      
      // データが正常に処理された場合、CANバスにデータを流し確認メッセージを表示する
      if (valueCount > 0) {
        char msg[20];
        CAN0.sendMsgBuf(canId, 0, payloadLength, payload);
        sprintf(msg, "Send --> 0x%03X ", canId);
        Serial.print(msg);
        sprintf(msg, "(%d) ", payloadLength);
        Serial.print(msg);
        for (int i = 0; i < payloadLength; i++) {
          sprintf(msg, "0x%02X ", payload[i]);
          Serial.print(msg);
        }
        Serial.println();
      }
      else {
        Serial.println("No valid values found");
      }
    }
  }
}

unsigned long parseHexString(String hexStr) {
  hexStr.trim();  // 前後の空白を削除
  
  if (hexStr.length() == 0) {
    return 0xFFFFFFFF;  // エラー値
  }
  
  // 16進数文字列を数値に変換
  unsigned long result = 0;
  for (int i = 0; i < hexStr.length(); i++) {
    char c = hexStr.charAt(i);
    result *= 16;
    
    if (c >= '0' && c <= '9') {
      result += c - '0';
    }
    else if (c >= 'A' && c <= 'F') {
      result += c - 'A' + 10;
    }
    else if (c >= 'a' && c <= 'f') {
      result += c - 'a' + 10;
    }
    else {
      return 0xFFFFFFFF;  // 無効な文字
    }
  }
  
  return result;
}
