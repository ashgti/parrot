@.LC0 = internal constant [13 x i8] c"Hello World\0A\00"

declare i32 @puts(i8*)

declare i8* @Parrot_new(i8*)
declare i8* @Parrot_get_params_pc(i8*, i8*)
declare void @Parrot_init_stacktop(i8*, i8**)
declare void @Parrot_set_config_hash()
declare i8* @Parrot_str_new(i8*, i8*, i64)
declare void @Parrot_set_executable_name(i8*, i8*)
declare void @Parrot_set_flag(i8*, i64)
declare i8* @PackFile_new(i8*, i64)

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind ssp {
    %cast210 = getelementptr [13 x i8]* @.LC0, i64 0, i64 0

    %interp = call i8* @Parrot_new(i8* null)

    %interp.addr = alloca i8*, align 4
    store i8* %interp, i8** %interp.addr
    call void @Parrot_init_stacktop(i8* %interp, i8** %interp.addr)
    call void @Parrot_set_config_hash()
    %exe_name.addr = load i8** %argv
    %exe_name = call i8* @Parrot_str_new(i8* %interp, i8* %exe_name.addr, i64 0)
    call void @Parrot_set_executable_name(i8* %interp, i8* %exe_name)
    call void @Parrot_set_flag(i8* %interp, i64 512)

    %pf = call i8* @PackFile_new(i8* %interp, i64 0)
    
    do_sub_progmas(interp )


    call i32 @puts(i8* %cast210)
    

    ret i32 0
}

!1 = metadata !{i32 41}
!foo = !{!1, null}
