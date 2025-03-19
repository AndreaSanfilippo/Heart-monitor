;*******************************************************************************
; Keyboard.pic - Module for reading a 4x4 keypad with PIC18F87K22
;*******************************************************************************
    
    #include <xc.inc>
    
    global  KeyPad_setup, KeyPad_read
    
;*******************************************************************************
; Variables in Access RAM
;*******************************************************************************
psect   udata_acs
KeyPad_row:     ds  1   ; Stores the row of pressed key
KeyPad_col:     ds  1   ; Stores the column of pressed key
KeyPad_temp:    ds  1   ; Temporary storage
KeyPad_result:  ds  1   ; Final keypad reading result
    
;*******************************************************************************
; Reset Vector
;*******************************************************************************
psect   code
    
;*******************************************************************************
; KeyPad_setup - Initializes ports for keypad operation
;*******************************************************************************
KeyPad_setup:
    ; Set BSR to access PADCFG1 register (not in Access RAM)
    movlb   0x0F            ; Bank 15 for special function registers
    
    ; Enable pull-up resistors for PORTE by setting REPU bit in PADCFG1
    bsf     PADCFG1, 7      ; Set REPU bit (bit 7 of PADCFG1)
    
    ; Clear LATE register - sets all output pins to 0
    clrf    LATE, a
    
    ; Configure initial state - PORTE[0:3] as inputs, PORTE[4:7] as outputs
    movlw   0x0F            ; 0000 1111 - 0-3 as inputs, 4-7 as outputs
    movwf   TRISE, a
    
    return
    
;*******************************************************************************
; KeyPad_read - Reads and decodes the pressed key on the keypad
; Returns key value in W
;   0-9: Number keys
;   10-15: Letter keys A-F (0x0A through 0x0F)
;   0x00: No key pressed
;   0xFF: Error/invalid key combination
;*******************************************************************************
KeyPad_read:
    ; Step 1: Configure for row detection and read
    call    KeyPad_read_rows
    movwf   KeyPad_row, a   ; Save row information
    
    ; Step 2: Configure for column detection and read
    call    KeyPad_read_cols
    movwf   KeyPad_col, a   ; Save column information
    
    ; Step 3: Decode the key based on row and column
    call    KeyPad_decode
    
    ; Return the result in W
    movf    KeyPad_result, w, a
    return
    
;*******************************************************************************
; KeyPad_read_rows - Detect which row has a key pressed
; Returns: W with row bit pattern (bit set to 0 if pressed)
;*******************************************************************************
KeyPad_read_rows:
    ; Configure PORTE[0:3] as inputs, PORTE[4:7] as outputs
    movlw   0x0F            ; 0000 1111 - 0-3 as inputs, 4-7 as outputs
    movwf   TRISE, a
    
    ; Drive all column outputs low
    movlw   0x00
    movwf   LATE, a
    
    ; Short delay for signals to settle
    call    KeyPad_delay
    
    ; Read row inputs from PORTE
    movf    PORTE, w, a     ; Read PORTE
    andlw   0x0F            ; Mask to get only bits 0-3 (rows)
    
    return
    
;*******************************************************************************
; KeyPad_read_cols - Detect which column has a key pressed
; Returns: W with column bit pattern (bit set to 0 if pressed)
;*******************************************************************************
KeyPad_read_cols:
    ; Configure PORTE[0:3] as outputs, PORTE[4:7] as inputs
    movlw   0xF0            ; 1111 0000 - 0-3 as outputs, 4-7 as inputs
    movwf   TRISE, a
    
    ; Drive all row outputs low
    movlw   0x00
    movwf   LATE, a
    
    ; Short delay for signals to settle
    call    KeyPad_delay
    
    ; Read column inputs from PORTE
    movf    PORTE, w, a     ; Read PORTE
    andlw   0xF0            ; Mask to get only bits 4-7 (columns)
    swapf   WREG, w, a      ; Swap nibbles to get column bits in lower nibble
    
    return
    
;*******************************************************************************
; KeyPad_decode - Decode row and column information to determine key pressed
; Uses: KeyPad_row, KeyPad_col
; Returns: Key value in KeyPad_result
;*******************************************************************************
KeyPad_decode:
    ; Initialize result to error
    movlw   0xFF
    movwf   KeyPad_result, a
    
    ; Check if no key pressed (all row and col bits = 1)
    movf    KeyPad_row, w, a
    xorlw   0x0F            ; XOR with 0x0F (all 1s)
    bnz     key_pressed     ; If not all 1s, a key is pressed
    
    movf    KeyPad_col, w, a
    xorlw   0x0F            ; XOR with 0x0F (all 1s)
    bnz     key_pressed     ; If not all 1s, a key is pressed
    
    ; No key pressed
    movlw   0x00
    movwf   KeyPad_result, a
    return
    
key_pressed:
    ; Get row index (0-3)
    movf    KeyPad_row, w, a
    call    KeyPad_find_first_zero
    movwf   KeyPad_row, a
    
    ; Get column index (0-3)
    movf    KeyPad_col, w, a
    call    KeyPad_find_first_zero
    movwf   KeyPad_col, a
    
    ; Calculate key value based on row and column
    ; Key = row * 4 + column
    movf    KeyPad_row, w, a
    mullw   4               ; Multiply row by 4
    movf    PRODL, w, a     ; Get lower byte of product
    addwf   KeyPad_col, w, a ; Add column
    
    ; Map to standard keypad layout:
    ; [1][2][3][F]
    ; [4][5][6][E]
    ; [7][8][9][D]
    ; [A][0][B][C]
    
    ; Use lookup table for mapping
    movwf   KeyPad_temp, a  ; Save calculated index
    movlw   low KeyPad_mapping
    movwf   TBLPTRL, a
    movlw   high KeyPad_mapping
    movwf   TBLPTRH, a
    movlw   upper KeyPad_mapping
    movwf   TBLPTRU, a
    
    ; Add offset
    movf    KeyPad_temp, w, a
    addwf   TBLPTRL, f, a
    btfsc   STATUS, 0, a    ; Check for carry
    incf    TBLPTRH, f, a
    
    ; Read value from table
    tblrd   *
    movf    TABLAT, w, a
    movwf   KeyPad_result, a
    
    return
    
;*******************************************************************************
; KeyPad_find_first_zero - Find the position of the first zero bit in W
; Returns: W with position (0-3)
;*******************************************************************************
KeyPad_find_first_zero:
    movwf   KeyPad_temp, a
    
    btfss   KeyPad_temp, 0
    retlw   0
    
    btfss   KeyPad_temp, 1
    retlw   1
    
    btfss   KeyPad_temp, 2
    retlw   2
    
    btfss   KeyPad_temp, 3
    retlw   3
    
    ; No zero found (should never happen if called correctly)
    retlw   0xFF
    
;*******************************************************************************
; KeyPad_delay - Short delay for signal settling
;*******************************************************************************
KeyPad_delay:
    ; Simple delay loop - adjust as needed
    movlw   50
delay_loop:
    nop
    nop
    decfsz  WREG, w, a
    bra     delay_loop
    return
    
;*******************************************************************************
; KeyPad mapping table - Maps the physical position to key value
;*******************************************************************************
psect   const
KeyPad_mapping:
    db      0x01, 0x02, 0x03, 0x0F  ; 1, 2, 3, F (row 0)
    db      0x04, 0x05, 0x06, 0x0E  ; 4, 5, 6, E (row 1)
    db      0x07, 0x08, 0x09, 0x0D  ; 7, 8, 9, D (row 2)
    db      0x0A, 0x00, 0x0B, 0x0C  ; A, 0, B, C (row 3)
    
end
