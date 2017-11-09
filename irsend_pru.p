
#define PRU0_ARM_INTERRUPT 19

.setcallreg r11.w0

.origin 0
.entrypoint Start

/*
PRU Data ram format - filled in by irsend.c
00 - r0    <repeats> - no of times to repeat sequence (simulating holding the button down)
04 - r1.w0 <pattern> - pronto pattern type - only 0000 types valid
06 - r1.w2 <carrier frequency> - code * 24.1246 / 2 - e.g. 40khz = 25us = 1250
08 - r2.w0 <burst1> - no of burst pairs in 1st sequence
10 - r2.w1 <burst2> - no of burst pairs in 2nd sequence
12 - r3.w0 <mark> - no of carrier cycles for 1st mark
14 - r3.w2 <space> - no of carrier cycles for 1st space
16 - r3.w0 <mark> - no of carrier cycles for 2sd mark
18 - r3.w2 <space> - no of carrier cycles for 2sd space
20 ... and so on

r4 used as a delay counter
r5 used as a data pointer
r6 used as a data pointer to the start of burst2 sequence
*/

.macro doCarrierClock
  MOV r4, r1.w2 ; clock high delay count
HighDelay:
  SUB r4, r4, 1
  QBNE HighDelay, r4, 0
  CLR r30, r30, 14 ; turn led <p8.12> off
  MOV r4, r1.w2 ; clock low delay count
LowDelay:
  SUB r4, r4, 1
  QBNE LowDelay, r4, 0
.endm



.macro doBurst
BurstLoop:
  LBBO r3, r5, 0, 8 ; load next mark/space pair
  ADD r5, r5, 4 ; advance data pointer
MarkDelay:
  SET r30, r30, 14 ; turn led <p8.12> on
  doCarrierClock
  SUB r3.w0, r3.w0, 1 ; mark count
  QBNE MarkDelay, r3.w0, 0
SpaceDelay:
  CLR r30, r30, 14 ; turn led <p8.12> off
  doCarrierClock
  SUB r3.w2, r3.w2, 1 ; space count
  QBNE SpaceDelay, r3.w2, 0
  SUB r2.w0, r2.w0, 1
  QBNE BurstLoop, r2.w0, 0 ; do next pair in burst
.endm


Start:
  MOV r5, 0
  LBBO r0, r5, 0, 12 ; load 8 32bit words into registers
  ADD r5, r5, 12; ; Make r5 point to first data word
  QBNE NotPronto, r1.w0, 0 ; only works for Pronto 0000 pattern
  QBEQ DoBurst2, r2.w0, 0 ; skip if no burst 1
  doBurst ; Do Burst 1
DoBurst2:
  QBEQ Finished, r2.w2, 0 ; skip if no burst 2
  MOV r6, r5 ; Save ptr to the start of burst 2
RepeatBurst2:
  MOV r2.w0, r2.w2 ; load the burst2 count
  MOV r5, r6 ; reset pointer back to start of burst 2
  doBurst ; Do Burst 2
  SUB r0, r0, 1
  QBNE RepeatBurst2, r0, 0 ; repeat burst2 as required
  JMP Finished

NotPronto:
JMP Finished
;QBNE NotUnmodulated, r1.w0, 0x0100
  ; Todo

NotUnmodulated:
;  QBNE NotRC5, r1.w0, 0x5000
  ; Todo

NotRC5:
;  QBNE NotRC5x, r1.w0, 0x5001
  ; Todo

NotRC5x:
;  QBNE NotRC6Mode0, r1.w0, 0x6000
  ; Todo

NotRC6Mode0:
;QBNE NotRaw, r1.w0, 0xffff


NotRaw:
Finished:
  MOV r8,0 ; Save the registers back to ram for debugging
  SBBO r0,r8,0,32
  MOV r31.b0, PRU0_ARM_INTERRUPT+16
  HALT
