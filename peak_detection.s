; peak_detection.s
; Heart Signal Peak Detection Algorithm for Negative Peaks
; Optimized for noise ~700mV and beat signal below 250mV
; PIC18F87K22 Processor

#include <xc.inc>
    
; Export public functions
global  PEAK_DETECTION_INIT, DETECT_PEAK
    
; Import ADC result
extrn  adc_result

; Define variables in uninitialized data section    
psect	udata
threshold:      ds 1       ; Voltage threshold for peak detection (250mV)
noise_level:    ds 1       ; Noise level threshold (700mV)
previous_value: ds 2       ; Previous ADC reading (16-bit)
min_value:      ds 2       ; Minimum value during current beat (16-bit)
state:          ds 1       ; State machine for peak detection
debounce:       ds 1       ; Debounce counter to prevent false detections

; State machine values
STATE_WAITING    equ 0     ; Waiting for signal to cross below beat threshold
STATE_SEARCHING  equ 1     ; Searching for minimum (peak)
STATE_RECOVERY   equ 2     ; Waiting for signal to recover above noise level

psect peak_code, class=CODE

; Peak Detection Initialization
PEAK_DETECTION_INIT:
    ; Set threshold for beat detection
    ; If using 4.096V reference: 250mV/4.096V * 1023 = ~62
    movlw  0x3E             ; 250mV on 4.096V scale (62 in decimal)
    movwf  threshold, A     ; Store threshold value in memory
    
    ; Set noise level threshold
    ; If using 4.096V reference: 700mV/4.096V * 1023 = ~175
    movlw  0xAF             ; 700mV on 4.096V scale (175 decimal)
    movwf  noise_level, A   ; Store noise level value in memory
    
    ; Initialize tracking variables
    clrf   previous_value, A    ; Clear previous value registers
    clrf   previous_value+1, A  ; Clear high byte of previous value
    clrf   min_value, A         ; Clear minimum value registers
    clrf   min_value+1, A       ; Clear high byte of minimum value
    
    ; Initialize state machine
    movlw  STATE_WAITING      ; Start in waiting state
    movwf  state, A           ; Store state in memory
    
    ; Clear debounce
    clrf   debounce, A        ; Clear debounce counter
    
    return                    ; Return from subroutine

; Detect Peak in Heart Signal
; Returns: Carry flag set if peak detected, clear otherwise
DETECT_PEAK:
    ; Check if we're in debounce period
    movf   debounce, W, A     ; Check if debounce counter is non-zero
    bnz    HANDLE_DEBOUNCE    ; If debounce is non-zero, handle debounce
    
    ; State machine for peak detection
    movf   state, W, A        ; Move state value to W register
    bz     STATE_WAITING_HANDLER ; If state is 0 (STATE_WAITING), branch to handler
    
    xorlw  STATE_SEARCHING    ; XOR W with STATE_SEARCHING
    bz     STATE_SEARCHING_HANDLER ; If result is 0, branch to SEARCHING handler
    
    xorlw  STATE_RECOVERY ^ STATE_SEARCHING  ; XOR again to check for STATE_RECOVERY
    bz     STATE_RECOVERY_HANDLER ; If result is 0, branch to RECOVERY handler
    
    ; Should never reach here
    bcf    STATUS, 0, A          ; Clear carry flag (no peak detected)
    return                    ; Return from subroutine

; Handle waiting state - looking for signal to drop below beat threshold
STATE_WAITING_HANDLER:
    ; Check if signal has fallen below beat threshold (250mV)
    movf   adc_result+1, W, A ; Move high byte of ADC result to W
    subwf  threshold, W, A    ; Subtract ADC result from threshold (threshold - ADC result)
    bn     SIGNAL_STILL_ABOVE_THRESHOLD  ; If result is negative, signal is above threshold
    
    ; Signal has crossed below beat threshold, move to searching state
    movlw  STATE_SEARCHING    ; Load searching state value
    movwf  state, A           ; Update state machine
    
    ; Initialize min value to current value
    movff  adc_result, min_value       ; Copy current ADC result to min_value
    movff  adc_result+1, min_value+1   ; Copy high byte
    
    ; No peak detected yet
    bcf    STATUS, 0, A          ; Clear carry flag
    bra    UPDATE_PREVIOUS_VALUE ; Update previous value and return
    
SIGNAL_STILL_ABOVE_THRESHOLD:
    ; Remain in waiting state
    bcf    STATUS, 0, A          ; Clear carry flag (no peak detected)
    bra    UPDATE_PREVIOUS_VALUE ; Update previous value and return

; Handle searching state - looking for minimum value
STATE_SEARCHING_HANDLER:
    ; Compare current value with min value
    movf   adc_result+1, W, A ; Move high byte of ADC result to W
    subwf  min_value+1, W, A  ; Subtract ADC result from min_value+1 (min_value - ADC result)
    bn     NEW_MINIMUM        ; If result is negative, we have a new minimum
    bz     CHECK_LOW_BYTE_MIN ; If high bytes equal, check low byte
    
    ; Current value is higher than min, check if rising significantly
    movf   previous_value+1, W, A ; Move high byte of previous value to W
    subwf  adc_result+1, W, A     ; Subtract previous from adc_result+1 (ADC - previous)
    bn     STILL_SEARCHING     ; If result is negative, still decreasing
    
    ; Rising edge detected, we've found a peak
    movlw  STATE_RECOVERY     ; Load recovery state value
    movwf  state, A           ; Update state machine
    
    ; Set debounce to prevent immediate re-detection
    movlw  50                 ; Adjust based on your sampling rate (decimal 50)
    movwf  debounce, A        ; Set debounce counter
    
    ; Signal peak detected
    bsf    STATUS, 0, A          ; Set carry flag (peak detected)
    bra    UPDATE_PREVIOUS_VALUE ; Update previous value and return
    
CHECK_LOW_BYTE_MIN:
    movf   adc_result, W, A   ; Move low byte of ADC result to W
    subwf  min_value, W, A    ; Subtract ADC result from min_value (min_value - ADC result)
    bn     NEW_MINIMUM        ; If result is negative, we have a new minimum
    
    ; Current value is higher than min, continue searching
    bra    STILL_SEARCHING
    
NEW_MINIMUM:
    ; Update minimum value
    movff  adc_result, min_value      ; Copy current ADC result to min_value
    movff  adc_result+1, min_value+1  ; Copy high byte
    
STILL_SEARCHING:
    ; No peak detected yet
    bcf    STATUS, 0, A          ; Clear carry flag
    bra    UPDATE_PREVIOUS_VALUE ; Update previous value and return

; Handle recovery state - waiting for signal to return above noise level
STATE_RECOVERY_HANDLER:
    ; Check if signal has risen above noise level (700mV)
    movf   noise_level, W, A  ; Move noise level to W
    subwf  adc_result+1, W, A ; Subtract noise_level from adc_result+1 (ADC - noise_level)
    bn     SIGNAL_STILL_BELOW_NOISE ; If result is negative, signal still below noise
    
    ; Signal has recovered above noise level, reset to waiting state
    movlw  STATE_WAITING      ; Load waiting state value
    movwf  state, A           ; Update state machine
    
SIGNAL_STILL_BELOW_NOISE:
    ; No peak detected during recovery
    bcf    STATUS, 0, A          ; Clear carry flag
    bra    UPDATE_PREVIOUS_VALUE ; Update previous value and return

; Update previous value for next iteration
UPDATE_PREVIOUS_VALUE:
    movff  adc_result, previous_value      ; Copy current ADC result to previous_value
    movff  adc_result+1, previous_value+1  ; Copy high byte
    return                                 ; Return from subroutine

; Handle debounce period
HANDLE_DEBOUNCE:
    decf   debounce, F, A     ; Decrement debounce counter
    bcf    STATUS, 0, A          ; Clear carry flag (no peak detected during debounce)
    return                    ; Return from subroutine

end