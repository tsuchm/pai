;円周率計算プログラム  ver5.0
;		by JN3GUM
brocks		=	4
;長桁数のデータ領域の必要数（exmath2.hで用いている。）
paper_width	=	80
;出力の時に何文字毎に改行コードを挿入するか、を規定する定数


		.xlist
		include	exmath2.h
		.list


		.386


;セグメント配置の定義
stack		segment	stack use16
stack		ends

data		segment use16
data		ends

code		segment	use16
code		ends

prg_tail	segment use16
prg_tail	ends



stack		segment	stack use16
		dw	64 dup(0)
stack		ends



data		segment use16

digit_number	dd	0
data_size	dw	0

		dw	46	;opn_msg のバイト長
opn_msg		db	0dh,0ah
		db	"円周率 π 計算プログラム ver5.0  by jn3gum"
		db	0dh,0ah

		dw	60	;inp_msg のバイト長
inp_msg		db	0dh,0ah
		db	"小数点以下の有効桁数を入力して下さい"
		db	"（１−６０００００）："

		dw	20	;inp_err_msg のバイト長
inp_err_msg	db	0dh,0ah
		db	"入力が不適切です。"

		dw	24	;mem_err_msg のバイト長
mem_err_msg	db	0dh,0ah
		db	"メモリが足りません。"
		db	0dh,0ah

		dw	30	;keika1 のバイト長
keika1		db	"２進数による演算を開始します。"

		dw	70	;keika2 のバイト長
keika2		db	"２進数による演算が終了しました。"
		db	0dh,0ah
		db	"２進数→１０進数の変換を開始します。"

		dw	38	;keika3 のバイト長
keika3		db	"２進数→１０進数の変換が終了しました。"

		dw	42	;time_output のバイト長
time_output	db	"（現在時刻："
date_buf	dw	0,0
		db	"年"
		dw	0
		db	"月"
		dw	0
		db	"日"
time_buf	dw	0
		db	"時"
		dw	0
		db	"分"
		dw	0
		db	"秒）",0dh,0ah

inp_buf		db	8 dup(0)

		dw	2
kaigyou		db	0dh,0ah

		dw	40	;get_err_msg のバイト長
get_err_msg	db	"標準入力から入力中にエラーが生じました。"

		dw	38	;out_err_msg のバイト長
out_err_msg	db	"計算結果を出力中にエラーが生じました。"

		dw	30
err_code	db	"エラーコードは、"
err_code_buf	dw	0,0
		db	"h です。",0dh,0ah

hensuu_table	dw	offset A
		dw	offset B
		dw	offset C
		dw	offset D

A		hensuu	<?,?>
B		hensuu	<?,?>
C		hensuu	<?,?>
D		hensuu	<?,?>

temp		hensuu	<?,?>
;hensuu構造体は長桁数のデータ領域を指すポインタや計算の途中経過の情報
;を格納するための構造体である。（exmath2.hで定義）

data		ends



code		segment	use16
		assume	cs:code,ds:data,es:data,ss:stack

start:
		mov	ax,data
		mov	ds,ax		;DS = data
		
		mov	dx,offset opn_msg
		call	put_err		;標準エラー出力に出力
		
;有効桁数の入力
inp_digit:
		mov	dx,offset inp_msg
		call	put_err		;標準エラー出力に出力
		call	get_std_8	;標準入力からの入力
		
		mov	bx,ax
		mov	cx,ax
		mov	al,inp_buf[bx-1]
		cmp	al,0ah
		jz	short @f	;入力ミスでなければジャンプ
		
		call	inp_err
inp_clear:	call	get_std_8	;余っている入力をクリアする
		jz	inp_clear
		jmp	inp_digit
		
;入力された文字コードを２進数に変換
@@:		sub	cx,2
		xor	eax,eax
		mov	ebx,10
		xor	si,si
		
chr2bin:
		mul	ebx
		or	edx,edx
		jz	short @f
		call	inp_err		;入力された数が大きすぎる
		jmp	inp_digit
@@:		mov	dl,inp_buf[si]
		sub	dl,'0'
		jns	short @f
		call	inp_err		;数字以外が入力された
		jmp	inp_digit
@@:		cmp	dl,10
		js	short @f
		call	inp_err		;数字以外が入力された
		jmp	inp_digit
@@:		add	eax,edx
		inc	si
		loop	chr2bin
		
		mov	digit_number,eax;有効桁数を設定
;data_size の計算
		mov	ebx,1000
		mul	ebx
		mov	ebx,9632
		div	ebx
		add	eax,3
		cmp	eax,65536
		jc	short @f
		call	inp_err
		jmp	inp_digit
@@:		mov	data_size,ax	;データ領域の大きさを設定
		
		mov	dx,offset kaigyou
		call	put_err		;標準エラー出力に出力
		
		shokika mem_err_msg
		;計算に必要なメモリを確保するマクロ命令（exmath2.hで定義）
		;メモリの確保に失敗した場合は、mem_err_msg で指定された
		;メッセージを出力してプログラムを終了する。
		
		mov	dx,offset keika1
		call	put_status



;π=48Arctan(1/18)+32Arctan(1/57)-20Arctan(1/239)
;48Arctan(1/18) の計算ルーチン
		mov	eax,48
		mov	ebx,18
		mov	di,offset A
		call	arctan

;32Arctan(1/57) の計算ルーチン
		mov	eax,32
		mov	ebx,57
		mov	di,offset D
		call	arctan

		tasizan	A,D		;A=A+D を行うマクロ命令

;20Arctan(1/239) の計算ルーチン
		mov	eax,20
		mov	ebx,239
		mov	di,offset D
		call	arctan

		hikizan	A,D		;A=A-D を行うマクロ命令

		mov	dx,offset keika2
		call	put_status


;A に格納されている円周率の２進数のデータを１０進数に変換して、B に転送する
		mov	ax,es:A._seg
		mov	ds,ax		;DS = A._seg
		mov	es:A.status,dword ptr 0
		
		mov	ax,es:B._seg
		mov	fs,ax		;FS = B._seg
		xor	edi,edi		;fs:[edi] -> Bを操作するためのポインタ
		
		mov	eax,ds:[ebp-4]	;整数部分のデータを取得
		mov	ds:[ebp-4],dword ptr 0
		add	al,'0'
		mov	fs:[edi],al
		inc	edi
		mov	fs:[edi],byte ptr '.'
		inc	edi
		
		mov	ebx,paper_width-2
		mov	ecx,es:digit_number	;有効桁数の設定
		
conv_loop:	push	ebx
		mov	esi,es:A.status
		
;末尾の０のデータ部分に対する処理の省略
bin2bcd_skip:	mov	eax,ds:[esi]
		add	esi,4
		or	eax,eax
		jz	short bin2bcd_skip
		
;無駄な検索を省くための処理
		sub	esi,4
		mov	es:A.status,esi
		xor	bx,bx
		
;データを１０倍するルーチン
bin2bcd_loop:	mov	eax,ds:[esi]
		rcl	bl,1		;BL = キャリーの保存
		rcl	eax,1		;EAX = データ×２
		rcl	bl,1
		mov	edx,eax
		rcl	edx,1
		rcl	bl,1
		rcl	edx,1		;EDX = データ×８
		rcl	bl,1
		adc	eax,edx		;EAX = データ×１０
		rcr	bl,1
		rcr	bl,1
		rcr	bl,1
		rcr	bl,1
		mov	ds:[esi],eax
		add	esi,4
		cmp	esi,ebp		;cmp	esi,data_size*4
		jnz	bin2bcd_loop
		
		pop	ebx
		
;BCDコードをASCIIコードに変換する処理
		mov	eax,ds:[ebp-4]
		mov	ds:[ebp-4],dword ptr 0
		add	al,'0'
		mov	fs:[edi],al
		inc	edi
		dec	ebx
		jnz	no_return
		
;改行コードを挿入する処理
		mov	fs:[edi],0a0dh
		add	edi,2
		mov	ebx,paper_width
		
no_return:	dec	ecx
		jz	conv_exit
		jmp	conv_loop
		
;２進数から１０進数への変換が終了
conv_exit:	mov	fs:[edi],0a0dh
		add	edi,2
		
		push	edi		;edi = 出力データ長
		
		mov	dx,offset keika3
		call	put_status
		
		pop	ebx		;ebx = 出力データ長
		mov	ax,es:B._seg
		mov	ds,ax
		xor	dx,dx		;ds:[dx] -> 出力するデータ
		mov	ecx,65520	;出力するデータ長
		
output_loop:	cmp	ebx,65520
		jb	short @f
		call	put_std
		sub	ebx,65520
		mov	ax,ds
		add	ax,65520/16
		mov	ds,ax
		jmp	output_loop
		
@@:		mov	ecx,ebx
		call	put_std
		
		xor	al,al		;errorlevel = 0 を設定
		
prg_exit:	shuuryou
		;確保した領域を全て解放してプログラムを終了させるマクロ命令



;α・arctanθを計算するサブルーチン
;	eax = α
;	ebx = １／θ
;	data:[di] -> 結果を出力するデータ領域を指示するhensuu構造体へのポインタ
arctan		proc
		push	di
		push	eax
		push	ebx
		mov	ax,data
		mov	ds,ax		;DS = data
		
;出力するデータ領域を指示するhensuu構造体をコピー
		cld
		mov	si,di
		mov	di,offset temp
		mov	cx,hensuu_length
		rep	movsb
		
;work領域の初期化
		mov	esi,ebp		;EBP = (data_size)*4
		sub	esi,4		;ESI = (data_size-1)*4
		mov	es:temp._offset,esi
		mov	es:B._offset,esi
		mov	es:C._offset,esi
		
;work領域とtempによって指示された領域に初項を代入
		pop	ebx		;EBX = １／θ
		pop	eax		;EAX = α
		xor	edx,edx
		
		mov	ds,es:temp._seg
		mov	fs,es:B._seg
shokou:
		div	ebx
		mov	ds:[esi],eax
		mov	fs:[esi],eax
		xor	eax,eax
		sub	esi,4
		jns	shokou
		
		mov	eax,ebx
		mul	ebx
		mov	ebx,eax		;公比を計算
		mov	ecx,1		;ECX = n
		
arctan_loop:	inc	ecx		;ECX = n
		warizan	B		;B=B/EBX を行うマクロ命令
		push	ebx		;公比を保存
		mov	ebx,ecx
		shl	ebx,1
		dec	ebx		;EBX = 2n-1
		warizan2 C,B,,arctan_exit
				;C=B/EBX を行うマクロ命令
				;演算を終了する場合は、arctan_exit へジャンプ
		pop	ebx
		test	ecx,1		;n の偶奇を判定
		jz	short guusuu	;n が偶数の時ジャンプ

;n = （奇数）の時のルーチン
kisuu:
		tasizan	temp,C		;temp=temp+C を行うマクロ命令
		jmp	arctan_loop

;n = （偶数）の時のルーチン
guusuu:
		hikizan	temp,C		;temp=temp-C を行うマクロ命令
		jmp	arctan_loop

;arctanを抜けるための処理
arctan_exit:	cld
		pop	ebx
		pop	di
		mov	ax,data
		mov	ds,ax
		mov	cx,hensuu_length
		mov	si,offset temp
		rep	movsb
		mov	es:B._offset,ebp	;work領域を解放する
		mov	es:C._offset,ebp
		ret

arctan		endp



;途中経過を報告するサブルーチン
;	data:[dx] -> 途中経過のメッセージ
put_status	proc
;途中経過のメッセージを標準エラー出力に出力
		mov	ax,data
		mov	ds,ax
		call	put_err		;標準エラー出力に出力
;現在時刻を標準エラー出力に出力
		mov	ah,2ah
		int	21h		;日付を取得
;月を処理
		mov	al,dh
		aam
		xchg	ah,al
		add	ax,3030h
		mov	date_buf+6,ax
;日を処理
		mov	al,dl
		aam
		xchg	ah,al
		add	ax,3030h
		mov	date_buf+10,ax
		
		mov	ax,cx
		xor	dx,dx
		mov	bx,100
		div	bx
		
;年（西暦）の上２桁を処理
		aam
		xchg	ah,al
		add	ax,3030h
		mov	date_buf,ax
;年（西暦）の下２桁を処理
		mov	ax,dx
		aam
		xchg	ah,al
		add	ax,3030h
		mov	date_buf+2,ax
		
		mov	ah,2ch
		int	21h		;時刻の取得
;時を処理
		mov	al,ch
		aam
		xchg	ah,al
		add	ax,3030h
		mov	time_buf,ax
;分を処理
		mov	al,cl
		aam
		xchg	ah,al
		add	ax,3030h
		mov	time_buf+4,ax
;秒を処理
		mov	al,dh
		aam
		xchg	ah,al
		add	ax,3030h
		mov	time_buf+8,ax
		
		mov	dx,offset time_output
		call	put_err		;標準エラー出力に出力
		ret			;return
put_status	endp



;入力ミスの時のメッセージ表示
inp_err		proc
		mov	dx,offset inp_err_msg
		call	put_err		;標準エラー出力に出力
		ret
inp_err		endp



;標準入力から８バイトのデータを取得するサブルーチン
;	inp_buf = 取得したデータ
;	ax = 実際に取得したデータのバイト数
;	入力時にエラーが生じた場合、エラーメッセージを出力して、強制終了
get_std_8	proc
		push	bx
		push	cx
		push	dx
		push	ds
		mov	ax,data
		mov	ds,ax
		mov	ah,3fh		;ファイル・ハンドルを用いた入力
		xor	bx,bx		;標準入力を指定
		mov	cx,8
		mov	dx,offset inp_buf
		int	21h
		jc	short get_std_err
		pop	ds
		pop	dx
		pop	cx
		pop	bx
		ret			;return
		
;入力中にエラーが生じたときの処理
get_std_err:	mov	dx,offset get_err_msg
		call	put_err
		call	put_err_code
		mov	al,1
		jmp	prg_exit	;プログラムの強制終了
		
get_std_8	endp



;標準出力に出力するサブルーチン
;	ds:[dx] -> 出力するデータ
;	cx = データの長さ
;	出力に失敗した場合、エラーメッセージを出力して、強制終了
put_std		proc
		push	ax
		push	bx
		mov	ah,40h		;ファイル・ハンドルを用いた出力
		mov	bx,1		;標準出力を指定
		int	21h
		jc	put_std_err
		pop	bx
		pop	ax
		ret			;return
		
;出力に失敗した場合の処理
put_std_err:	mov	dx,offset out_err_msg
		call	put_err
		call	put_err_code
		mov	al,3
		jmp	prg_exit	;強制終了
		
put_std		endp



;標準エラー出力に出力するサブルーチン
;	data:[dx] -> メッセージ
;	data:[dx-2] -> メッセージの長さ
;	出力中にエラーが生じた場合、強制終了
put_err		proc
		push	ax
		push	bx
		push	cx
		push	ds
		mov	ax,data
		mov	ds,ax
		mov	bx,dx
		mov	cx,[bx-2]	;メッセージの長さを設定
		mov	ah,40h		;ファイル・ハンドルを用いた出力
		mov	bx,2		;標準エラー出力を指定
		int	21h
		jc	put_err_err
		pop	ds
		pop	cx
		pop	bx
		pop	ax
		ret			;return
		
put_err_err:	mov	al,2
		jmp	prg_exit	;強制終了
		
put_err		endp



;エラーコードを標準エラー出力に出力するサブルーチン
;	ax = エラーコード
;	segment rezister 以外の全ての rezister の内容は保証されない。
put_err_code	proc
		push	ds
		push	ax
		push	ax
		mov	ax,data
		mov	ds,ax
		pop	ax
		xchg	ah,al
		call	byte2hex
		mov	err_code_buf,ax
		pop	ax
		call	byte2hex
		mov	err_code_buf+2,ax
		mov	dx,offset err_code
		call	put_err
		pop	ds
		ret
put_err_code	endp



;1byte のデータを 2byte の１６進文字列に変換するサブルーチン
;	al = 変換するデータ
;ret	ax = 変換された文字列
byte2hex	proc
		push	cx
		mov	cx,4
		xor	ah,ah
		ror	ax,cl
		shr	ah,cl
		add	ax,3030h
		cmp	ah,'9'
		jna	@f
		add	ah,7
@@:		cmp	al,'9'
		jna	@f
		add	al,7
@@:		pop	cx
		ret
byte2hex	endp



code		ends

		end	start
