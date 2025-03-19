; ADC.s
#include <xc.inc>
#include <xc.inc>

global  ADC_Setup, ADC_Read, adc_result    
    
; uninitialized global variables
psect   udata_acs
adc_result:  ds 2  ; Reserve 2 bytes for the 16-bit ADC result
    
psect	adc_code, class=CODE
    
ADC_Setup:
	bsf	TRISA, PORTA_RA0_POSN, A  ; pin RA0==AN0 input
	movlb	0x0f
	bsf	ANSEL0	    ; set AN0 to analog
	movlb	0x00
	movlw   0x01	    ; select AN0 for measurement
	movwf   ADCON0, A   ; and turn ADC on
	movlw   0x30	    ; Select 4.096V positive reference
	movwf   ADCON1,	A   ; 0V for -ve reference and -ve input
	movlw   0xF6	    ; Right justified output
	movwf   ADCON2, A   ; Fosc/64 clock and acquisition times
	return

ADC_Read:
	bsf	GO	    ; Start conversion by setting GO bit in ADCON0
adc_loop:
	btfsc   GO	    ; check to see if finished
	bra	adc_loop
	
	; Save the result to adc_result
	movf   ADRESL, W, A           ; Load low byte of ADC result (ADRESL)
	movwf  adc_result, A          ; Store in low byte of adc_result
	movf   ADRESH, W, A           ; Load high byte of ADC result (ADRESH)
	movwf  adc_result+1, A        ; Store in high byte of adc_result
	return

end