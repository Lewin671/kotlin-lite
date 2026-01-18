; ModuleID = 'kotlin_lite'
source_filename = "kotlin_lite"

define void @main() {
entry:
  br i1 true, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  call void @print_i32(i32 30)
  br label %if.merge

if.else:                                          ; preds = %entry
  call void @print_i32(i32 -10)
  br label %if.merge

if.merge:                                         ; preds = %if.else, %if.then
  ret void
}

declare void @print_i32(i32)
