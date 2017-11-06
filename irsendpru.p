
#define PRU0_ARM_INTERRUPT 19

.setcallreg r11.w0

.origin 0
.entrypoint Start 

/*
;.struct SignalDesc     Offset
;.u32 repeats ;   r0 	0 no of times to repeat burst sequence 2 (simulating holding the button down)
;.u16 pattern ;  r1.w0	4
;.u16 freq ;	 r1.w2	6
;.u16 burst1 ;   r2.w0	8
;.u16 burst2 ; 	 r2.w2	10
;.u32 mark   ;   r3.w0  12
;.u32 space  ;   r3.w2  14
; r5 index into codes
*/

.macro doCarrierClock
  MOV r4, r1.w2
HighDelay:
  SUB r4, r4, 1
  QBNE HighDelay, r4, 0
  CLR r30, r30, 14 ; clear P8.12
  MOV r4, r1.w2
LowDelay:
  SUB r4, r4, 1
  QBNE LowDelay, r4, 0
.endm



.macro doBurst
BurstLoop:
  LBBO r3, r5, 0, 8 ; load
  ADD r5, r5, 4;
MarkDelay:
  SET r30, r30, 14 ; set P8.12
  doCarrierClock
  SUB r3.w0, r3.w0, 1
  QBNE MarkDelay, r3.w0, 0
SpaceDelay:
  CLR r30, r30, 14 ; clear P8.12
  doCarrierClock
  SUB r3.w2, r3.w2, 1
  QBNE SpaceDelay, r3.w2, 0
  SUB r2.w0, r2.w0, 1
  QBNE BurstLoop, r2.w0, 0
.endm


Start:
  MOV r5, 0
  LBBO r0, r5, 0, 12 ; load 8 32bit words into registers
  ADD r5, r5, 12; // Make r5 point to first data word
  QBNE NotPronto, r1.w0, 0
  doBurst // Do Burst 1
  MOV r6, r5
RepeatBurst2:  
  MOV r2.w0, r2.w2 // reset the burst2 count   
  MOV r5, r6 // reset pointer back to start of burst 2
  doBurst // Do Burst 2
  SUB r0, r0, 1
  QBNE RepeatBurst2, r0, 0
  JMP Finished     

NotPronto:

Finished:
  MOV r8,0
  SBBO r0,r8,0,32
  MOV r31.b0, PRU0_ARM_INTERRUPT+16
  HALT

