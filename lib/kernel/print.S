TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data
put_int_buffer dq 0		;定义8byte缓冲区用于数字到字符的转换

[bits 32]
section .text

;---------put_str------------
;通过put_char打印以0结尾的字符串
;栈中存放的是字符串的地址


global put_str
put_str:
	push ebx
	push ecx
	xor ecx, ecx
	mov ebx, [esp + 12]		;从栈中得到待打印字符串的地址
.goon:
	mov cl, [ebx]
	cmp cl, 0
	jz .str_over
	push ecx		;传参
	call put_char
	add esp, 4		;回收栈
	inc ebx
	jmp .goon
.str_over:
	pop ecx
	pop ebx
	ret

;-------put_char---------
;把栈中的一个字符写入光标所在处
;------------------------
global put_char
put_char:
	pushad		;备份8个32位寄存器
	mov ax, SELECTOR_VIDEO
	mov gs, ax

;获取当前光标位置
	;先获得高八位
	mov dx, 0x03d4		;索引寄存器端口号		
	mov al, 0x0e		;0eh为cursor location high register的索引
	out dx, al
	mov dx, 0x03d5		;从这个端口读数据	
	in al, dx
	mov ah, al

	;获取低八位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx

	;此时ax已经是光标的坐标了
	mov bx, ax
	;在栈中获取被打印字符
	mov ecx, [esp + 36]		;pushad压了32byte + 4byte的返回地址

	cmp cl, 0xd
	jz .is_carriage_return		;这个字符是不是回车
	cmp cl, 0xa
	jz .is_line_feed		;换行
	cmp cl, 0x8
	jz .is_backspace		;backsapce
	
	jmp .put_other		;打印字符
	

.is_backspace:
	dec bx
	shl bx, 1		;左移一位相当于乘以2

	mov byte [gs:bx], 0x20
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	jmp .set_cursor

.put_other:
	shl bx, 1
	
	mov byte [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx		;光标指向下一个应该写入的位置
	;若光标的位置小于2000, 则不用滚屏
	cmp bx, 2000
	jl .set_cursor


;这里把回车换行合并起来操作
.is_line_feed:
.is_carriage_return:
	xor dx, dx
	mov bx, ax
	mov si, 80
	div si
	sub bx, dx		;光标值减去处以80的余数便是去整,即回车
	
.is_carriage_return_end:
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:
	jl .set_cursor

;滚屏的原理是将第1~24行复制到0~23行
;第24行用空格填充
.roll_screen:
	cld
	mov ecx, 960	;一次要搬运2000 - 80 = 1920个字符 共1920 * 2 = 3840byte                     ;一次搬运4byte 3840 / 4 = 960次
	mov esi, 0xc00b80a0
	mov edi, 0xc00b8000
	rep movsd
	;填充最后一行
	mov ebx, 3840
	mov ecx, 80
.cls:
	mov word [gs:ebx], 0x0720
	add ebx, 2
	loop .cls
	mov bx, 1920	;将光标置为最后一行的首字符

;之前只是计算出了光标的位置，要把它写入相应端口才会生效
.set_cursor:
	;先设置高八位
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al
	
	;低八位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.put_char_done:
	popad
	ret

;--------将小端字节序的数字变成对应的ascii，倒置-----------
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印十六进制数字

global put_int
put_int:
	pushad
	mov ebp, esp
	mov eax, [ebp + 4 * 9]
	mov edx, eax
	mov edi, 7		;指定在put_int_buffer中初始的偏移量
	mov ecx, 8		;32位数字中, 十六进制数字的位数是8个
	mov ebx, put_int_buffer

;将32位数按照十六进制的形式从低到高逐个处理
.16based_4bits:
	and edx, 0x0000000F
	cmp edx, 9		;0~9, A~F要分开处理
	jg .is_A2F
	add edx, '0'
	jmp .store

.is_A2F:
	sub edx, 10
	add edx, 'A'

;将每一个数字转变成字符后，把低位字符放在高低值
;即，因为上面从32位数的后四位开始转换的，把转换后对应的ascii从
;缓冲区的最后一个字节开始填，这样符合人的习惯
.store:
	mov [ebx + edi], dl		;此时dl中是数字对应的ascii
	dec edi
	shr eax, 4
	mov edx, eax
	loop .16based_4bits

;现在put_int_buffer中已经全是字符, 但是00001234应该打印成1234才对
.ready_to_print:
	inc edi		;经过上面的遍历 edi = -1，+1使其为0
.skip_prefix_0:
	cmp edi, 8
	je .full0	;表示待打印的字符全是0
;找出连续的0字符，edi作为非0的最高为字符的偏移
.go_on_skip:
	mov cl, [put_int_buffer + edi]
	inc edi
	cmp cl, '0'
	je .skip_prefix_0
	dec edi
	jmp .put_each_num

.full0:
	mov cl, '0'
.put_each_num:
	push ecx	;传参
	call put_char
	add esp, 4
	inc edi
	mov cl, [put_int_buffer + edi]
	cmp edi, 8
	jl .put_each_num
	popad
	ret
