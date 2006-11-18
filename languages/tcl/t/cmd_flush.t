#!../../parrot tcl.pbc

source lib/test_more.tcl
plan 3

eval_is {flush} \
  {wrong # args: should be "flush channelId"} \
  {no args}

eval_is {flush the monkeys} \
  {wrong # args: should be "flush channelId"} \
  {too many args}

eval_is {flush toilet} \
  {can not find channel named "toilet"} \
  {invalid channel name}

# RT#40627: test actual flushing.
