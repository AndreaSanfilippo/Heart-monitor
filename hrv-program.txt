;*******************************************************************************
; HRV.asm - Program to read HRV input from keypad and display on LCD
; for PIC18F87K22
;*******************************************************************************
    
    #include <xc.inc>
    
    ; External function declarations for LCD and Keypad modules
    extrn   LCD_Setup, LCD_Clear, LCD_Set_Position, LCD_Send_Byte_D
    extrn   LCD_Write_Message, LCD_Write_Message_PM
    extrn   KeyPad_setup, KeyPad_read
    
    ; Export global variables
    global  HRV_value
    
;*******************************************************************************
; Variables in Access RAM
;*******************************************************************************
psect   udata_acs
HRV_value:      ds  1       ; Global variable to store HRV value (0-99)
Keypad_val:     ds  1       ; Temporary storage for keypad reading
First_digit:    ds  1       ; First digit of HRV value
Second_digit:   ds  1       ; Second digit of HRV value
Counter:        ds  1       ; Counter for message display
    
;*******************************************************************************
; Reset Vector
;*******************************************************************************
psect   code, abs
reset_vec:
    org     0x00            ; Reset vector
    goto    start
    
;*******************************************************************************
; Main Program
;*******************************************************************************
psect   code
start:
    ; Initialize the device
    call    Initialize
    
    ; Clear the LCD
    call    LCD_Clear
    
    ; Display prompt message
    call    Display_Prompt
    
    ; Read HRV input from keypad
    call    Read_HRV
    
    ; Display the entered HRV value
    call    Display_HRV
    
    ; Main loop
main_loop:
    goto    main_loop       ; Loop indefinitely
    
;*******************************************************************************
; Initialize - Set up the device and peripherals
;*******************************************************************************
Initialize:
    ; Set up the LCD
    call    LCD_Setup
    
    ; Set up the keypad
    call    KeyPad_setup
    
    ; Initialize HRV value to 0
    clrf    HRV_value, a
    
    return
    
;*******************************************************************************
; Display_Prompt - Display the prompt message on the LCD
;*******************************************************************************
Display_Prompt:
    ; Set cursor to beginning of first line
    movlw   0x00
    call    LCD_Set_Position
    
    ; Display message "Insert critical HRV:"
    movlw   low Prompt_msg
    movwf   TBLPTRL, a
    movlw   high Prompt_msg
    movwf   TBLPTRH, a
    movlw   upper Prompt_msg
    movwf   TBLPTRU, a
    
    movlw   19              ; Message length
    call    LCD_Write_Message_PM
    
    return
    
;*******************************************************************************
; Read_HRV - Read a two-digit HRV value from the keypad (00-99)
;*******************************************************************************
Read_HRV:
    ; Read first digit
Read_First:
    call    KeyPad_read
    movwf   Keypad_val, a
    
    ; Check if valid key (0-9)
    movlw   0x0A            ; Compare with 10 (first value after 9)
    cpfslt  Keypad_val, a   ; Skip if Keypad_val < 10
    bra     Read_First      ; If not valid digit, try again
    
    ; Check if a key was pressed
    movf    Keypad_val, w, a
    bz      Read_First      ; If no key pressed, try again
    
    ; Store first digit
    movwf   First_digit, a
    
    ; Calculate decimal value (first digit * 10)
    mullw   10              ; Multiply by 10
    movff   PRODL, HRV_value ; Store in HRV_value
    
    ; Display first digit
    movlw   0x13            ; Position after prompt
    call    LCD_Set_Position
    
    movf    First_digit, w, a
    addlw   '0'             ; Convert to ASCII
    call    LCD_Send_Byte_D
    
    ; Wait for key release
Key_Release1:
    call    KeyPad_read
    bnz     Key_Release1
    
    ; Short delay
    movlw   100
    call    Delay_ms
    
    ; Read second digit
Read_Second:
    call    KeyPad_read
    movwf   Keypad_val, a
    
    ; Check if valid key (0-9)
    movlw   0x0A            ; Compare with 10 (first value after 9)
    cpfslt  Keypad_val, a   ; Skip if Keypad_val < 10
    bra     Read_Second     ; If not valid digit, try again
    
    ; Check if a key was pressed
    movf    Keypad_val, w, a
    bz      Read_Second     ; If no key pressed, try again
    
    ; Store second digit
    movwf   Second_digit, a
    
    ; Add second digit to HRV_value
    movf    Second_digit, w, a
    addwf   HRV_value, f, a
    
    ; Display second digit
    movlw   0x14            ; Position after first digit
    call    LCD_Set_Position
    
    movf    Second_digit, w, a
    addlw   '0'             ; Convert to ASCII
    call    LCD_Send_Byte_D
    
    ; Wait for key release
Key_Release2:
    call    KeyPad_read
    bnz     Key_Release2
    
    return
    
;*******************************************************************************
; Display_HRV - Display the entered HRV value on the LCD
;*******************************************************************************
Display_HRV:
    ; Set cursor position after prompt
    movlw   0x13            ; Position after prompt
    call    LCD_Set_Position
    
    ; Display first digit
    movf    HRV_value, w, a
    movwf   Keypad_val, a   ; Use Keypad_val as temporary storage
    
    ; Divide by 10 to get first digit
    movlw   10
    clrf    WREG, a
    movlw   10
    divf    Keypad_val, w, a
    
    ; Display first digit
    addlw   '0'             ; Convert to ASCII
    call    LCD_Send_Byte_D
    
    ; Calculate second digit (remainder)
    movf    HRV_value, w, a
    movwf   Keypad_val, a
    
    ; Get remainder after division by 10
    movlw   10
    movwf   Counter, a
    clrf    WREG, a
Get_Remainder:
    movf    Counter, w, a
    subwf   Keypad_val, f, a
    bc      Get_Remainder
    
    ; Add back the 10 (oversubtracted)
    movlw   10
    addwf   Keypad_val, f, a
    
    ; Display second digit
    movf    Keypad_val, w, a
    addlw   '0'             ; Convert to ASCII
    call    LCD_Send_Byte_D
    
    return
    
;*******************************************************************************
; Delay_ms - Delay for given number of milliseconds
; Input: W contains number of milliseconds to delay
;*******************************************************************************
Delay_ms:
    movwf   Counter, a      ; Store delay count
    
Delay_ms_loop:
    ; Delay for 1ms (adjust for your clock frequency)
    movlw   250
    call    Delay_us
    
    decfsz  Counter, f, a
    bra     Delay_ms_loop
    
    return
    
;*******************************************************************************
; Delay_us - Delay for given number of microseconds
; Input: W contains number of microseconds to delay
;*******************************************************************************
Delay_us:
    movwf   Counter, a      ; Store delay count
    
Delay_us_loop:
    ; 4-cycle loop = 1us @ 4MHz
    nop
    decfsz  Counter, f, a
    bra     Delay_us_loop
    
    return
    
;*******************************************************************************
; Constant data
;*******************************************************************************
psect   const
Prompt_msg:
    db      "Insert critical HRV:"
    
end
