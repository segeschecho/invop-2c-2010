\ x0 - x1-+
\ |  \ |  |
\ x2 - x3 |
\ '-------+

Minimize
 obj: w0 + w1 + w2 + w3
Subject To
 soloUnColorNodo0: x00 + x01 + x02 + x03 = 1
 soloUnColorNodo1: x10 + x11 + x12 + x13 = 1
 soloUnColorNodo2: x20 + x21 + x22 + x23 = 1
 soloUnColorNodo3: x30 + x31 + x32 + x33 = 1
 adyDeDistintoColor001: x00 + x10 - w0 <= 0
 adyDeDistintoColor101: x01 + x11 - w1 <= 0
 adyDeDistintoColor201: x02 + x12 - w2 <= 0
 adyDeDistintoColor301: x03 + x13 - w3 <= 0
 adyDeDistintoColor002: x00 + x20 - w0 <= 0
 adyDeDistintoColor102: x01 + x21 - w1 <= 0
 adyDeDistintoColor202: x02 + x22 - w2 <= 0
 adyDeDistintoColor302: x03 + x23 - w3 <= 0
 adyDeDistintoColor003: x00 + x30 - w0 <= 0
 adyDeDistintoColor103: x01 + x31 - w1 <= 0
 adyDeDistintoColor203: x02 + x32 - w2 <= 0
 adyDeDistintoColor303: x03 + x33 - w3 <= 0
 adyDeDistintoColor013: x10 + x30 - w0 <= 0
 adyDeDistintoColor113: x11 + x31 - w1 <= 0
 adyDeDistintoColor213: x12 + x32 - w2 <= 0
 adyDeDistintoColor313: x13 + x33 - w3 <= 0
 adyDeDistintoColor023: x20 + x30 - w0 <= 0
 adyDeDistintoColor123: x21 + x31 - w1 <= 0
 adyDeDistintoColor223: x22 + x32 - w2 <= 0
 adyDeDistintoColor323: x23 + x33 - w3 <= 0
 adyDeDistintoColor012: x10 + x20 - w0 <= 0
 adyDeDistintoColor112: x11 + x21 - w1 <= 0
 adyDeDistintoColor212: x12 + x22 - w2 <= 0
 adyDeDistintoColor312: x13 + x23 - w3 <= 0
Binary
 w0 w1 w2 w3 x00 x01 x02 x03 x10 x11 x12 x13 x20 x21 x22 x23 x30 x31 x32 x33
End