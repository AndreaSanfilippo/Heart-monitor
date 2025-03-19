;*******************************************************************************
; LCD.pic - Module for interfacing with 2x16 LCD display on PIC18F87K22
;*******************************************************************************
    
    #include <xc.inc>
    
    global  LCD_Setup, LCD_Write_Message, LCD_Send_Byte_D, LCD_Send_Byte_I
    global  LCD_Clear, LCD_Set_Position, LCD_Write_Message_PM
    
;*******************************************************************************
; Constants
;*******************************************************************************
; LCD pins on PORTB
LCD_RS      EQU 4   ; Register Select on RB4
LCD_E       EQU 5   ; Enable on RB5
 
;*******************************************************************************
; Variables in Access RAM
;*******************************************************************************
psect   udata_acs
LCD_tmp:        ds  1   ; Temporary storage
LCD_counter:    ds  1   ; Counter for message loop
LCD_cnt_l:      ds  1   ; Low byte for delay counter
LCD_cnt_h:      ds  1   ; High byte for delay counter
LCD_line_pos:   ds  1   ; Position storage for LCD line
    
;*******************************************************************************
; LCD Control Functions
;*******************************************************************************
psect   code
    
;*******************************************************************************
; LCD_Setup - Initialize the LCD display
;*******************************************************************************
LCD_Setup:
    ; Configure PORTB for LCD
    clrf    LATB, a       ; Clear PORTB output latches
    movlw   0xC0          ; RB0:5 all outputs (b11000000)
    movwf   TRISB, a
    
    ; Wait 40ms for LCD to start up properly
    movlw   40
    call    LCD_delay_ms
    
    ; Function set 4-bit
    movlw   0x30          ; 00110000B
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    ; 2 line display 5x8 dot characters
    movlw   0x28          ; 00101000B
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    ; Repeat, 2 line display 5x8 dot characters
    movlw   0x28          ; 00101000B
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    ; Display on, cursor on, blinking on
    movlw   0x0F          ; 00001111B
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    ; Display clear
    movlw   0x01          ; 00000001B
    call    LCD_Send_Byte_I
    
    ; Wait 2ms
    movlw   2
    call    LCD_delay_ms
    
    ; Entry mode increment by 1 no shift
    movlw   0x06          ; 00000110B
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    return
    
;*******************************************************************************
; LCD_Send_Byte_I - Send a byte to the LCD instruction register
; Input: W contains byte to send
;*******************************************************************************
LCD_Send_Byte_I:
    movwf   LCD_tmp, a    ; Store byte temporarily
    
    ; Send high nibble first
    swapf   LCD_tmp, w, a ; Swap nibbles, high nibble goes first
    andlw   0x0F          ; Select just low nibble
    movwf   LATB, a       ; Output data bits to LCD
    bcf     LATB, LCD_RS, a ; Instruction write, clear RS bit
    call    LCD_Enable    ; Pulse enable bit
    
    ; Send low nibble next
    movf    LCD_tmp, w, a ; Get original byte, now do low nibble
    andlw   0x0F          ; Select just low nibble
    movwf   LATB, a       ; Output data bits to LCD
    bcf     LATB, LCD_RS, a ; Instruction write, clear RS bit
    call    LCD_Enable    ; Pulse enable bit
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    return
    
;*******************************************************************************
; LCD_Send_Byte_D - Send a byte to the LCD data register
; Input: W contains byte to send
;*******************************************************************************
LCD_Send_Byte_D:
    movwf   LCD_tmp, a    ; Store byte temporarily
    
    ; Send high nibble first
    swapf   LCD_tmp, w, a ; Swap nibbles, high nibble goes first
    andlw   0x0F          ; Select just low nibble
    movwf   LATB, a       ; Output data bits to LCD
    bsf     LATB, LCD_RS, a ; Data write, set RS bit
    call    LCD_Enable    ; Pulse enable bit
    
    ; Send low nibble next
    movf    LCD_tmp, w, a ; Get original byte, now do low nibble
    andlw   0x0F          ; Select just low nibble
    movwf   LATB, a       ; Output data bits to LCD
    bsf     LATB, LCD_RS, a ; Data write, set RS bit
    call    LCD_Enable    ; Pulse enable bit
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    return
    
;*******************************************************************************
; LCD_Enable - Pulse the enable line to latch data
;*******************************************************************************
LCD_Enable:
    bsf     LATB, LCD_E, a ; Set enable bit
    nop                    ; Short delay
    nop
    bcf     LATB, LCD_E, a ; Clear enable bit
    return
    
;*******************************************************************************
; LCD_Clear - Clear the LCD display
;*******************************************************************************
LCD_Clear:
    movlw   0x01          ; Clear display command
    call    LCD_Send_Byte_I
    
    ; Wait 2ms - clear command needs a longer delay
    movlw   2
    call    LCD_delay_ms
    
    return
    
;*******************************************************************************
; LCD_Set_Position - Set the cursor position
; Input: W contains the desired position:
;        0x00-0x0F: First line positions 0-15
;        0x40-0x4F: Second line positions 0-15
;*******************************************************************************
LCD_Set_Position:
    movwf   LCD_line_pos, a
    iorlw   0x80          ; Set bit 7 to indicate DDRAM address command
    call    LCD_Send_Byte_I
    
    ; Wait 40us
    movlw   10
    call    LCD_delay_x4us
    
    return
    
;*******************************************************************************
; LCD_Write_Message - Write a message from data memory to LCD
; Input: FSR2 points to the message in data memory
;        W contains the length of the message
;*******************************************************************************
LCD_Write_Message:
    movwf   LCD_counter, a
    
LCD_Loop_message:
    movf    POSTINC2, w, a
    call    LCD_Send_Byte_D
    decfsz  LCD_counter, a
    bra     LCD_Loop_message
    
    return
    
;*******************************************************************************
; LCD_Write_Message_PM - Write a message from program memory to LCD
; Input: TBLPTR points to the start of message in program memory
;        W contains the length of the message
;*******************************************************************************
LCD_Write_Message_PM:
    movwf   LCD_counter, a
    
LCD_Loop_Message_PM:
    tblrd   *+
    movf    TABLAT, w, a
    call    LCD_Send_Byte_D
    decfsz  LCD_counter, a
    bra     LCD_Loop_Message_PM
    
    return
    
;*******************************************************************************
; Delay Routines
;*******************************************************************************

;*******************************************************************************
; LCD_delay - Basic delay routine 
;*******************************************************************************
LCD_delay:
    ; 4 instruction loop = 250ns
    movlw   0x00        ; W=0
LCD_delay_loop:
    decf    LCD_cnt_l, f, a  ; No carry when 0x00 -> 0xFF
    subwfb  LCD_cnt_h, f, a  ; No carry when 0x00 -> 0xFF
    bc      LCD_delay_loop   ; Carry, then loop again
    return                   ; Carry reset so return
    
;*******************************************************************************
; LCD_delay_x4us - Delay in multiples of 4 microseconds
; Input: W contains number of 4us delays
;*******************************************************************************
LCD_delay_x4us:
    movwf   LCD_cnt_l, a   ; Store W in counter low byte
    clrf    LCD_cnt_h, a   ; Clear counter high byte
    
    ; Each loop takes 1us at 4MHz (1 TCY), so we need to repeat 4 times for 4us
    movlw   16            ; 4us * 4 = 16 loop iterations
    mulwf   LCD_cnt_l, a   ; Multiply by 16
    movff   PRODL, LCD_cnt_l
    movff   PRODH, LCD_cnt_h
    call    LCD_delay
    return
    
;*******************************************************************************
; LCD_delay_ms - Delay in milliseconds
; Input: W contains number of milliseconds to delay
;*******************************************************************************
LCD_delay_ms:
    movwf   LCD_tmp, a     ; Store W in temporary variable
    
LCD_delay_ms_loop:
    ; Each millisecond is 1000us, which is 250 * 4us
    movlw   250
    call    LCD_delay_x4us
    decfsz  LCD_tmp, f, a
    bra     LCD_delay_ms_loop
    return
    
end
