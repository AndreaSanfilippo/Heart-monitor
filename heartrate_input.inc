;*******************************************************************************
; HeartRate.pic - Module for getting heart rate input from keypad and displaying
; on LCD for PIC18F87K22
;*******************************************************************************
    
    #include <xc.inc>
    
    ; Import external functions from LCD and Keypad modules
    extrn   LCD_Setup, LCD_Write_Message, LCD_Send_Byte_D, LCD_Send_Byte_I
    extrn   LCD_Clear, LCD_Set_Position, LCD_Write_Message_PM
    extrn   KeyPad_setup, KeyPad_read
    
    ; Export our functions and variables
    global  HeartRate_Setup, HeartRate_Input, HeartRate_Get
    global  HeartRate_Value
    
;*******************************************************************************
; Variables in Access RAM
;*******************************************************************************
psect   udata_acs
HeartRate_Value:    ds  1   ; Global variable to store heart rate
HR_digit_count:     ds  1   ; Counter for number of digits entered
HR_temp:            ds  1   ; Temporary storage for calculations
HR_first_digit:     ds  1   ; First digit entered
HR_second_digit:    ds  1   ; Second digit entered
HR_third_digit:     ds  1   ; Third digit entered
    
;*******************************************************************************
; Constants and Messages
;*******************************************************************************
psect   const
HR_message:
    db      'Insert Heartrate', 0    ; Null-terminated message
    
;*******************************************************************************
; HeartRate Functions
;*******************************************************************************
psect   code
    
;*******************************************************************************
; HeartRate_Setup - Initialize the LCD and Keypad for heart rate input
;*******************************************************************************
HeartRate_Setup:
    ; Initialize LCD
    call    LCD_Setup
    
    ; Initialize Keypad
    call    KeyPad_setup
    
    ; Clear heart rate value
    clrf    HeartRate_Value, a
    
    return
    
;*******************************************************************************
; HeartRate_Input - Get heart rate input from keypad
;*******************************************************************************
HeartRate_Input:
    ; Clear LCD display
    call    LCD_Clear
    
    ; Display "Insert Heartrate" message
    movlw   0x00            ; First line, position 0
    call    LCD_Set_Position
    
    ; Set up pointer to message
    movlw   low HR_message
    movwf   TBLPTRL, a
    movlw   high HR_message
    movwf   TBLPTRH, a
    movlw   upper HR_message
    movwf   TBLPTRU, a
    
    ; Display message
    movlw   16              ; Length of message
    call    LCD_Write_Message_PM
    
    ; Set cursor to second line for input
    movlw   0x40            ; Second line, position 0
    call    LCD_Set_Position
    
    ; Initialize digit count
    clrf    HR_digit_count, a
    clrf    HR_first_digit, a
    clrf    HR_second_digit, a
    clrf    HR_third_digit, a
    
    ; Get heart rate input
    call    HR_Get_Input
    
    ; Store final value in global variable
    movf    HeartRate_Value, w, a
    
    return
    
;*******************************************************************************
; HR_Get_Input - Get keypad input and process it
;*******************************************************************************
HR_Get_Input:
    ; Reset heart rate value
    clrf    HeartRate_Value, a
    
HR_input_loop:
    ; Check if maximum digits entered (3 digits)
    movlw   3
    cpfslt  HR_digit_count, a
    return                  ; If 3 digits already entered, return
    
    ; Wait for key press
    call    KeyPad_read
    
    ; Check if no key pressed
    bz      HR_input_loop   ; If W = 0, no key pressed, continue waiting
    
    ; Check if key is a number (0-9)
    movwf   HR_temp, a
    movlw   0x0A            ; 10 decimal
    cpfslt  HR_temp, a
    bra     HR_input_loop   ; Skip if not a number
    
    ; We have a valid digit, process it
    
    ; First check if this would make the HR too high (>200)
    movf    HR_digit_count, w, a
    bz      HR_store_digit  ; If first digit, always accept
    
    ; Check if exceeding 200
    movf    HR_first_digit, w, a
    sublw   2               ; 2 - first_digit
    bn      HR_store_digit  ; If first digit > 2, accept any second digit
    
    ; First digit is 2, check if second digit would make it exceed 200
    movf    HR_digit_count, w, a
    xorlw   1               ; Check if we're entering second digit
    bnz     HR_check_third  ; If not second digit, check third digit
    
    ; We're entering second digit when first is 2
    movf    HR_temp, w, a
    sublw   0               ; 0 - second_digit
    bn      HR_store_digit  ; If second digit > 0, accept any third digit
    
    ; If we get here, starting with 20x, always accept
    bra     HR_store_digit
    
HR_check_third:
    ; Check if we're entering third digit when first is 2 and second is 0
    movf    HR_second_digit, w, a
    bnz     HR_store_digit  ; If second digit is not 0, accept any third digit
    
    ; We're entering third digit when first is 2 and second is 0
    movf    HR_temp, w, a
    sublw   0               ; 0 - third_digit
    bn      HR_input_loop   ; If third digit > 0, value would be >200, reject
    
HR_store_digit:
    ; Store the digit in the appropriate variable
    movf    HR_digit_count, w, a
    bz      HR_first        ; If count = 0, store first digit
    xorlw   1
    bz      HR_second       ; If count = 1, store second digit
    
    ; Store third digit
    movf    HR_temp, w, a
    movwf   HR_third_digit, a
    bra     HR_update_display
    
HR_first:
    movf    HR_temp, w, a
    movwf   HR_first_digit, a
    bra     HR_update_display
    
HR_second:
    movf    HR_temp, w, a
    movwf   HR_second_digit, a
    
HR_update_display:
    ; Increment digit count
    incf    HR_digit_count, f, a
    
    ; Display the digit on LCD
    movf    HR_temp, w, a
    addlw   '0'             ; Convert to ASCII
    call    LCD_Send_Byte_D
    
    ; Calculate current heart rate value
    call    HR_Calculate_Value
    
    ; Wait for key release
    call    HR_Wait_Key_Release
    
    ; Continue input loop
    bra     HR_input_loop
    
;*******************************************************************************
; HR_Wait_Key_Release - Wait until key is released
;*******************************************************************************
HR_Wait_Key_Release:
HR_release_loop:
    call    KeyPad_read
    bnz     HR_release_loop ; If W != 0, key still pressed
    return
    
;*******************************************************************************
; HR_Calculate_Value - Calculate heart rate value from digits
;*******************************************************************************
HR_Calculate_Value:
    ; Reset heart rate value
    clrf    HeartRate_Value, a
    
    ; Process first digit
    movf    HR_first_digit, w, a
    movwf   HeartRate_Value, a
    
    ; Check if we have a second digit
    movf    HR_digit_count, w, a
    sublw   1               ; 1 - count
    bc      HR_calc_done    ; If count <= 1, we're done
    
    ; Multiply first digit by 10
    movlw   10
    mulwf   HeartRate_Value, a
    movff   PRODL, HeartRate_Value
    
    ; Add second digit
    movf    HR_second_digit, w, a
    addwf   HeartRate_Value, f, a
    
    ; Check if we have a third digit
    movf    HR_digit_count, w, a
    sublw   2               ; 2 - count
    bc      HR_calc_done    ; If count <= 2, we're done
    
    ; Multiply value by 10
    movlw   10
    mulwf   HeartRate_Value, a
    movff   PRODL, HeartRate_Value
    
    ; Add third digit
    movf    HR_third_digit, w, a
    addwf   HeartRate_Value, f, a
    
HR_calc_done:
    return
    
;*******************************************************************************
; HeartRate_Get - Get the current heart rate value
; Returns: W contains the heart rate value
;*******************************************************************************
HeartRate_Get:
    movf    HeartRate_Value, w, a
    return
    
end
