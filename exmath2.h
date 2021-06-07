;****************************
;長桁数演算用ヘッダファイル
;	３８６用のコードを使用している
;	データ領域の最高位の 2 word を整数とする固定小数点演算方式
;	EXE 形式のプログラムでのみ使用可能である。
;****************************



;****************************
;このヘッダファイルを使用するときは、以下の２つの定数を指定しなければならない。
;	brocks		確保するデータ領域の数
;	paper_width	出力する時に何文字毎に改行コードを挿入するか、を規定。
;****************************



;****************************
;このヘッダファイルを利用するときは、
;以下の３つのデータ領域を定義しておかなければならない。
;	digit_number	小数点以下の有効桁数（double word）
;			data_size*32*log2 以上の桁については保証されない。
;	data_size	データ格納領域のダブル・ワード長（word）
;	hensuu_table	データ領域を利用するためのhensuu構造体を指すポインタの
;			テーブル（word*brocks）
;****************************



;****************************
;変数定義用構造体
;	このヘッダファイルに収録された計算用のマクロ群は、
;	hennsuu 構造体を用いて定義された変数を扱う。
;	_seg は、変数のセグメント・アドレスが格納される。
;	_seg:[_offset] は、０以外の数が格納されているデータ領域を指すポインタ
;	である。無駄な計算や検索を省くために用いられる。
;	_offset に data_size*4 の値が代入されている時、その領域は未使用である。
;	status は、変数の使用状況が格納される。
hensuu		struc
_seg		dw	?		;;変数のセグメント・アドレスを格納
_offset		dd	0		;;０以外のデータが格納されている
					;;最も高位のメモリのオフセット
status		dd	?		;;変数の使用状況
hensuu		ends

hensuu_length	=	10		;;hensuu構造体のバイト長
;****************************



;****************************
;設計規則
;	EDI = ポインタ
;	ESI = ポインタ
;	EBP = data_size*4 = 定数
;
;
;	CS = コード・セグメント（マクロ中では変化しない）
;	DS = 最もアクセスの多いデータのセグメント
;	ES = データ・セグメント（マクロ中では変化しない）
;	FS = ２番目にアクセスの多いデータのセグメント
;	GS = 任意
;	SS = スタック・セグメント（マクロ中では変化しない）
;
;
;	ESI を操作した後は、必ず ESI の範囲の確認を行う。
;		例１：	add	esi,4
;			cmp	esi,data_size*4
;			jnz	@b
;
;		例２：	sub	esi,4
;			jns	@b
;
;
;	adc,sbb,div の各命令の後方で ESI を操作する。
;
;
;	出来るだけ使うレジスタを減らす。
;	使う順序は、EAX,EDX,EBX,ECX とする。
;
;
;****************************






;****************************
;初期化用マクロ
;	必要なメモリを確保し、hensuu_table によって指示された hensuu 構造体を
;	初期化する。
;	メモリ確保に失敗した場合、$err_msg で指定された文字列を標準エラー出力に
;	出力して、プログラムを終了する。
;	prg_tail の名前のセグメントが最後に配置されていなければならない。
;
;	本マクロ終了後、レジスタの値は次のように変化する。
;		EBP = data_size*4
;		CS = code
;		DS = data
;		ES = data
;		SS = stack
;	これ以外のレジスタの値は不定である。
;****************************
shokika		macro	$err_msg
		local	abc,bbb,err
		mov	ax,data
		mov	ds,ax		;;DS = data segment
		
		mov	ax,word ptr data_size
		shl	eax,1
		shl	eax,1
		mov	ebp,eax		;;EBP = data_size*4
		
		mov	ah,62h		;;PSPのセグメントを得る
		int	21h
		mov	es,bx		;;ES = PSP のセグメント・アドレス
		mov	ax,bx
		mov	bx,prg_tail	;;prg_tail = 末端のセグメント
		sub	bx,ax		;;BX = 使用する領域のパラグラフ長
		mov	ah,4ah		;;メモリ・ブロックの変更
		int	21h
		
		mov	ax,word ptr ds:data_size
		shr	ax,1
		shr	ax,1
		inc	ax
		push	ax
			;;データ領域１つあたりのパラグラフ長を算出
		mov	cx,brocks	;;確保する領域の数を設定
		mul	cx		;;AX = データ領域全体のパラグラフ長
		or	dx,dx
		jnz	short err
			;;MS-DOS で指定できる範囲を越えていたらジャンプ
		mov	bx,ax		;;bx = データ領域全体のパラグラフ長
		mov	ah,48h		;;メモリ・ブロックの割当
		int	21h		;;system call
		jnc	short abc	;;メモリ確保に成功したらジャンプ
err:
		pop	ax
		lea	dx,$err_msg	;;出力文字列を指定
		call	put_err		;;標準エラー出力に出力
		mov	ax,4c01h	;;プログラムの終了
		int	21h
		
abc:
		pop	bx		;;BX = データ領域１つのパラグラフ長
		mov	si,offset hensuu_table
bbb:
		mov	di,[si]		;;[DI] -> hensuu構造体
		mov	word ptr [di],ax	;;hensuu._seg を設定
		mov	dword ptr [di+2],ebp	;;hensuu._offset を設定
		mov	dword ptr [di+6],0	;;hensuu.status を設定
		add	ax,bx
			;;次のデータ領域のセグメント・アドレスの算出
		add	si,2
		loop	bbb
		
		mov	ax,data
		mov	es,ax		;;ES = data
		
		endm





;****************************
;終了マクロ
;	確保したメモリを全て解放して終了する。
;	al = errorlevel の状態で用いる。
;****************************
shuuryou	macro
		push	ax
		mov	ax,data
		mov	ds,ax		;;DS = data
		mov	ah,49h		;;メモリ・ブロックの解放
		mov	bx,hensuu_table
		mov	es,[bx]		;;解放するメモリ・ブロックの
					;;セグメント・アドレスを指定
		int	21h
		
		pop	ax
		mov	ah,4ch		;;プログラムの終了
		int	21h
		
		endm





;****************************
;変換用サブルーチン
;	２進数のデータを１０進数に変換するときに、
;	用いるサブルーチンのマクロ。
;	指定されたデータを１０倍する。
;Usage	$conv_sub exit
;	データがなくなったときのジャンプ先を指定して用いる。
;argument
;	DS = 指定されたデータのセグメント
;	ES = データ・セグメント
;return value
;	EAX = 整数部分のデータ
;
;	EAX,ESI は破壊される。
;	使用例については、shuturyoku マクロを参照。
;****************************
$conv_sub	macro	$label
		local	eee,fff,ggg
		push	ebx
		push	ecx
		mov	esi,es:stack_esi

;;末尾の０のデータ部分に対する処理の省略
eee:
		mov	eax,ds:[esi]
		or	eax,eax
		jnz	short fff	;;０以外のデータが存在していたらジャンプ
		add	esi,4
		cmp	esi,ebp		;;cmp	esi,data_size*4
		jnz	short eee
		jmp	$label		;;データがなくなったらジャンプ

;;無駄な検索を省くための処理
fff:
		mov	es:stack_esi,esi
		xor	bx,bx

;;指定されたデータを１０倍するルーチン
ggg:
		mov	eax,ds:[esi]
		rcl	bl,1		;;BL = キャリーの保存
		rcl	eax,1		;;EAX = データ×２
		rcl	bl,1
		mov	edx,eax
		rcl	edx,1
		rcl	bl,1
		rcl	edx,1		;;EDX = データ×８
		rcl	bl,1
		adc	eax,edx		;;EAX = データ×１０
		rcr	bl,1
		rcr	bl,1
		rcr	bl,1
		rcr	bl,1
		mov	ds:[esi],eax
		add	esi,4
		cmp	esi,ebp		;;cmp	esi,data_size*4
		jnz	ggg		;;データがなくなったらジャンプ
		
		pop	ecx
		pop	ebx
		
		endm





;****************************
;出力用マクロ
;	二進数のデータを十進数の文字列データに変換して、
;	標準出力に出力するマクロ。
;	このマクロは、shuturyoku_buf マクロと組み合わせて使用する。
;	shuturyoku_buf マクロでは、このマクロで使用するバッファを確保する。
;Usage	shuturyoku A,B
;	A で指定されたデータを、B で指定された領域を用いて出力する。
;****************************
shuturyoku	macro	A,B
		local	ddd,eee,fff,ggg,wr_data,wr_data_ret,owari
		mov	ds,es:A._seg	;;DS = 指定されたデータのセグメント
		mov	eax,ds:[ebp-4]	;;整数部分のデータを取得
		add	al,'0'		;;文字コードへの変換
		mov	es:wr_buf[0],al
		mov	ds:[ebp-4],dword ptr 0
					;;整数部分のデータの消去
		mov	ebx,2
		mov	ecx,es:digit_number
					;;有効桁数の設定

;;BIN → BCD 変換ルーチン
ddd:
		$conv_sub owari
		
		add	al,30h		;;文字コードへ変換
		mov	es:wr_buf[ebx],al
		mov	ds:[ebp-4],dword ptr 0
					;;整数部分のデータの消去
					;;EDI = 整数部分に対するポインタ
		inc	ebx
		cmp	ebx,paper_width	;;紙幅による制限
		jz	short wr_data	;;出力ルーチン呼出
wr_data_ret:
		dec	ecx		;;有効桁数による制限
		jz	short owari	;;既定の桁数の出力が終了したらジャンプ
		jmp	short ddd	;;ループ

;;標準出力への出力処理
wr_data:
		push	ecx
		mov	ax,data
		mov	ds,ax		;;DS = data
		mov	ah,40h		;;ファイル・ハンドルを用いた出力
		mov	bx,1		;;標準出力を指定
		mov	cx,82		;;文字数は 82 byte
		lea	dx,wr_buf
		int	21h		;;system call
		xor	ebx,ebx
		mov	ds,es:A._seg	;;DS = 指定されたデータのセグメント
		pop	ecx
		jmp	short wr_data_ret

;;２進数から１０進数への変換の終了
owari:
		mov	ax,data
		mov	ds,ax		;;DS = data
		mov	wr_buf[ebx],0dh
		inc	ebx
		mov	wr_buf[ebx],0ah	;;改行コードの埋め込み
		inc	ebx
		mov	cx,bx		;;文字数の設定
		mov	ah,40h		;;ファイル・ハンドルを用いた出力
		mov	bx,1		;;標準出力を指定
		lea	dx,wr_buf
		int	21h		;;system call
		
		endm



shuturyoku_buf	macro
wr_buf		db	'0.',(paper_width-2) dup('0'),0dh,0ah
stack_esi	dd	0
		endm





;****************************
;加算用マクロ	A=A+B
;****************************
tasizan		macro	A,B
		local	abc,bbb,yyy,zzz
		mov	edi,es:B._offset
		mov	ds,word ptr es:A._seg
		mov	fs,word ptr es:B._seg
		xor	dx,dx
		xor	esi,esi
		
;;メイン足し算ルーチン
abc:		rcl	dx,1		;;DX = キャリーの保存
		mov	eax,ds:[esi]	;;mov 命令は、CF を変更しない。
		adc	eax,fs:[esi]	;;キャリーを含んだ足し算
		mov	ds:[esi],eax
		rcr	dx,1
		add	esi,4
		cmp	edi,esi
		jns	short abc	;;ESI > B._offset の時ジャンプ
		
		cmp	esi,ebp		;;cmp	esi,data_size*4
		jz	short yyy	;;計算終了
		
;;繰り上がりの処理
bbb:		rcl	dx,1
		jnc	short yyy
		add	dword ptr ds:[esi],1
		rcr	dx,1
		add	esi,4
		cmp	esi,ebp		;;データの末端を検出
		jnz	short bbb
		
;;変数の状況変数の調節
yyy:		sub	esi,4
		mov	eax,es:A._offset;;データの存在範囲を更新
		cmp	eax,esi
		jns	short zzz
		mov	es:A._offset,esi
		
zzz:		
		endm





;****************************
;減算用マクロ	A=A-B
;****************************
hikizan		macro	A,B
		local	abc,bbb,yyy,zzz
		mov	edi,es:B._offset
		mov	ds,word ptr es:A._seg
		mov	fs,word ptr es:B._seg
		xor	esi,esi
		xor	dx,dx
		
;;メイン引き算ルーチン
abc:		rcl	dx,1		;;DX = キャリーの保存
		mov	eax,ds:[esi]
		sbb	eax,fs:[esi]	;;キャリーを含む引き算
		mov	ds:[esi],eax
		rcr	dx,1
		add	esi,4
		cmp	edi,esi
		jns	short abc
		
		cmp	esi,ebp		;;cmp	esi,data_size*4
		jz	short yyy
		
;;繰り下がりの調節
bbb:		rcl	dx,1		;;DX = キャリーの保存
		jnc	short yyy
		sub	dword ptr ds:[esi],1
		rcr	dx,1
		add	esi,4
		cmp	esi,ebp		;;データの末端の検出
		jnz	short bbb
		
;;変数の状況変数の調節
yyy:		sub	esi,4
		mov	eax,es:A._offset
		cmp	eax,esi
		jns	short zzz
		mov	es:A._offset,esi
zzz:		
		endm





;****************************
;割り算用マクロ	A=A/B
;	EBX に除数を代入して用いる事もできる。
;
;	本マクロ終了後のレジスタの値は次のようになっている。
;		EBX = B（整数）
;		ECX = 変化しない
;		CS = code（変化しない）
;		DS = A のセグメント・アドレス
;		ES = data（この状態で用いることを前提としている。）
;		SS = stack（変化しない）
;		EBP = data_size*4（変化しない）
;	これ以外のレジスタの値は不定である。
;****************************
warizan		macro	A,B
		local	abc,bbb,ccc,zzz
		mov	esi,es:A._offset
		
		ifnb	<B>
		mov	ebx,dword ptr B	;;除数をセット
		endif
		
		mov	ds,word ptr es:A._seg
		xor	edx,edx

;;_offset を調節する機能を盛り込んだ割り算ルーチン
abc:		mov	eax,ds:[esi]
		div	ebx
		mov	ds:[esi],eax
		or	eax,eax
		jnz	short bbb	;;EAX <> 0 ならば、ジャンプ
		sub	esi,4
		jns	short abc	;;ESI >= 0 ならば、割り算続行

		jmp	short zzz

;;_offset を調節する
bbb:
		mov	es:A._offset,esi
		sub	esi,4
		js	short zzz
		
;;純粋な割り算ルーチン
ccc:
		mov	eax,ds:[esi]
		div	ebx		;;割り算
		mov	ds:[esi],eax
		sub	esi,4
		jns	short ccc	;;ESI >= 0 ならば、割り算続行
		
zzz:					;;終了
		endm





;****************************
;割り算用マクロ Part2	A=B/C
;	EBX に除数を代入して用いる事もできる。
;	$exit が指定されている時に、（被除数）＝０で終了した場合、
;	$exit にジャンプして終了する。
;
;	本マクロ終了後のレジスタの値は次のようになっている。
;		EBX = C（整数）
;		ECX = 変化しない
;		CS = code（変化しない）
;		DS = A のセグメント・アドレス
;		ES = data（この状態で用いることを前提としている。）
;		FS = B のセグメント・アドレス
;		SS = stack（変化しない）
;		EBP = data_size*4（変化しない）
;	これ以外のレジスタの値は不定である。
;****************************
warizan2	macro	A,B,C,$exit
		local	abc,bbb,ccc,zzz
		mov	ds,word ptr es:A._seg
		mov	fs,word ptr es:B._seg
		mov	esi,dword ptr es:A._offset
		mov	eax,dword ptr es:B._offset
		xor	edx,edx
		
		ifnb	<C>
		mov	ebx,dword ptr C
		endif
		
		cmp	esi,eax
		jns	short ddd
		mov	esi,eax
		jmp	short abc

;;Ａのデータの内、余分な部分を消去する
ddd:
		mov	ds:[esi],dword ptr 0
		cmp	esi,eax
		jz	short abc
		sub	esi,4
		jns	short ddd

;;_offset を調節する機能を盛り込んだ割り算ルーチン
abc:
		mov	eax,fs:[esi]
		div	ebx
		mov	ds:[esi],eax
		or	eax,eax
		jnz	short bbb	;;EAX <> 0 ならば、ジャンプ
		sub	esi,4
		jns	short abc	;;ESI >= 0 ならば、割り算続行

		ifnb	<$exit>
		jmp	$exit
		else
		mov	es:A.status,dword ptr 1
		jmp	short zzz	;;（被除数）＝０ で終了
		endif

;;_offset の調節
bbb:
		mov	es:A._offset,esi
		sub	esi,4
		js	short zzz
		
;;純粋な割り算ルーチン
ccc:		mov	eax,fs:[esi]
		div	ebx		;;割り算
		mov	ds:[esi],eax
		sub	esi,4
		jns	short ccc	;;ESI >= 0 ならば、割り算続行
zzz:
		endm





;****************************
;代入用マクロ	A=B
;****************************
dainyuu		macro	A,B
		local	abc
		mov	dword ptr es:A._offset,ebp
		mov	esi,ebp
		sub	esi,4		;;ESI = (data_size-1)*4
		mov	ds,word ptr es:A._seg
		mov	ds:[esi],dword ptr B
		sub	esi,4
		
abc:		mov	ds:[esi],dword ptr 0
		sub	esi,4
		jns	abc
		
		endm





;****************************
;コピー用マクロ	A=B
;****************************
copy		macro	A,B
		local	abc
		mov	eax,es:B._offset
		mov	es:A._offset,eax
		mov	eax,es:B.status
		mov	es:A.status,eax
		mov	fs,es:A._seg
		mov	ds,es:B._seg
		mov	esi,ebp
		sub	esi,4		;;ESI = (data_size-1)*4
		
abc:		mov	eax,ds:[esi]
		mov	fs:[esi],eax
		sub	esi,4
		jns	abc
		
		endm




