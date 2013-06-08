#!/usr/bin/perl -w

# --- hash keys ---
# client: sckt, id, iaddr, port, in_buf, stable, final, played_move
#         best_move, nodes, value, pid
#
# score:  move, value, nodes, final, stable
# -----------------

# load perl modules
use strict;
use warnings;
use IO::Socket;
use IO::Select;
use IO::Handle;
use IO::File;
use Time::HiRes qw( time );
use File::Spec;
use Getopt::Long;

# autoflush STDOUT
$| = 1;

# constants
sub timeout    ()    { 180.0 } # keepalive time
sub send_freq  ()    {   0.5 }
sub maxeval_score () { 30_000 }

# subroutines
sub array_remove (&$);
sub wait_messages($$$$$$);
sub get_client_from_key($$);
sub change_evaluation_value($;$);
sub update_godwhale($);
sub update_current_info($$);
sub parse_smsg($$$);
sub parse_cmsg($$$);
sub parse_clogin($$$);
sub sort_clients($$);
sub get_line ($);
sub next_log ($);
sub out_log ($$);
sub out_clients ($$$);


my $global_pid = 0;
my @client = ();
my @movelist = ();
my $g_eval_value = 0;
my $g_update_lasttime = time();
my $g_need_update = 0;


{
    # defaults of command-line options
    my ( %status ) = ( client_port      => 4084,
                       client_num       => 3,
                       server_host      => 'localhost',
                       server_port      => 4082,
                       server_id        => 'Parallel1',
                       server_buf       => q{},
                       server_send_time => time(),
                       server_send_mmsg => q{},
                       split_nodes      => 100_000,
                       final            => 1,
                       sent_final       => 0,
                       stable           => 1,
                       server_pid       => 0,
		       myturn           => undef,
		       ponder_move      => undef,
                       fh_log           => undef );

    # parse command-line options
    GetOptions( \%status,
                'client_port=i',
                'client_num=i',
                'server_host=s',
                'server_port=i',
                'server_id=s',
                'split_nodes=i' ) or die "$!";

    # creates a listening socket for my clients.
    my $listening_sckt
        = new IO::Socket::INET( LocalPort => $status{client_port},
                                Listen    => SOMAXCONN,
                                Proto     => 'tcp',
                                ReuseAddr => 1 )
        or die "Can't create a listening socket: $!\n";

    print "wait for $status{client_num} clients:\n";

    change_evaluation_value \%status, 0;

    my ( $server_sckt, $count );
    while ( 1 ) {
        # connect to server
	$server_sckt
	    = new IO::Socket::INET( PeerAddr => $status{server_host},
				    PeerPort => $status{server_port},
				    Proto    => 'tcp' );
	if ( $server_sckt ) {
	    print $server_sckt "$status{server_id} 1.0 final stable\n";
	    last;
	}
            
	if ( $count ) { print "."; }
	else          { print "connect() to server faild, try again "; }
	$count += 1;

	sleep(1);
    }

    # main loop
    my $server_buf       = q{};
    my $server_send_time = time;
    while ( 1 ) {
        
        my ( @ready_keys) = wait_messages( \%status, $listening_sckt, \@client,
                                           $server_sckt, \$server_buf,
                                           timeout );
        my $time = time();

        foreach my $ready_key ( @ready_keys ) {

            if ( $ready_key eq 'server socket' ) {
                my $line = get_line \$server_buf;
                if ( not parse_smsg \%status, \@client, $line ) {
                    die "MESSAGE FROM SERVER: $line";
                }
            }
            else {
		my $this_client = $ready_key;
                my $line = get_line \$this_client->{in_buf};

                if ( not parse_cmsg \%status, $this_client, $line ) {
                    die "MESSAGE FROM CLIENT: $line\n";
                }
            }
        }

        # 必要ならクライアントをソートします。
        if ( $status{need_client_sort} ) {
	    sort_clients \%status, \@client;
            delete $status{need_client_sort};
	}

        update_godwhale \%status;
        update_current_info \%status, \@client;

        # no more messages to server after 'final'
        if ( $status{sent_final} ) {

            # send a null line for keepalive
            if ( not @ready_keys ) {
                out_log \%status, "Server< \n";
                print $server_sckt "\n";
            }
            next;
        }


        # organize client opinion into %score
        my $have_unexpanded    = 0;
        my ( %score )          = ();
        my ( @expanded_moves ) = ();
        foreach my $client_ref ( @client ) {
            
            my ( $move, $value, $expanded );

            if ( defined $client_ref->{played_move} ) {
                $move     =  $client_ref->{played_move};
                $value    = -$client_ref->{value};
                $expanded = 1;
                push @expanded_moves, $move;
            }
            elsif ( defined $client_ref->{best_move} ) {
                $move     = $client_ref->{best_move};
                $value    = $client_ref->{value};
                $expanded = 0;
                $have_unexpanded = 1;
            }
            else { next; }

            my $client_pid = $client_ref->{pid};
            if ( exists $score{$client_pid} ) {

                if ( $score{$client_pid}->{final} ) { next; }
                elsif ( not $client_ref->{final}
                        and ( $client_ref->{nodes}
                              <= $score{$client_pid}->{nodes} ) ) { next; }

            }

            $score{$client_pid} = { move       => $move,
                                    value      => $value,
                                    client_ref => $client_ref,
				    pv         => $client_ref->{pv},
                                    nodes      => $client_ref->{nodes},
                                    final      => $client_ref->{final},
                                    stable     => $client_ref->{stable},
                                    expanded   => $expanded };
        }


        # send a message to the server if necessary
        if ( keys %score ) {
            my ( $best_ref, $best_value, $final, $stable_str, $final_str );
	    my ( $pv_str );
            my $node_sum  = 0;
            my $moves_num = 0;
            my $stable    = 1;
            foreach my $score_ref ( values %score ) {
                $node_sum += $score_ref->{nodes};
                $stable   &= $score_ref->{stable};

                # count legal moves at root
                if ( $score_ref->{value} < - maxeval_score()
                     and $score_ref->{final} )   { $moves_num += 0; }
                elsif ( $score_ref->{expanded} ) { $moves_num += 1; }
                else                             { $moves_num += 100; }

                # find the best child
                if ( not defined $best_value
                     or $best_value < $score_ref->{value} ) {
                    $best_ref   = $score_ref;
                    $best_value = $best_ref->{value};
                }
            }
            
            if ( $best_ref->{final}
                 or ( $moves_num < 2 and $have_unexpanded ) ) { $final = 1; }
            else                                              { $final = 0; }

            if ( $status{stable} and $stable ) { $stable_str = ' stable'; }
            else                               { $stable_str = q{}; }

            if ( $status{final} and $final )   { $final_str = ' final'; }
            else                               { $final_str = q{}; }

            my $mmsg =( "pid=$status{server_pid} move=$best_ref->{move} "
                        . "v=${best_value}e$stable_str$final_str" );

            if ( defined $status{server_send_mmsg}
                 and $status{server_send_mmsg} eq $mmsg ) {
                
                if ( $status{server_send_time} + send_freq < $time ) {
                    my $line = "pid=$status{server_pid} n=$node_sum";
                    out_log \%status, "Server< $line\n";
                    print $server_sckt "$line\n";
                    $status{server_send_time} = $time;
                    if ( $final_str ) { $status{sent_final} = 1; }
                }
            }
            else {
                my $line = "$mmsg n=$node_sum";
                out_log \%status, "Server< $line\n";
                print $server_sckt "$line\n";
                $status{server_send_time} = $time;
                $status{server_send_mmsg} = $mmsg;
                if ( $final_str ) { $status{sent_final} = 1; }

                # PVの送信
                if ( $best_ref->{pv} ) {
                    my $value001 = ${best_value} * 0.01;
                    my $tmp = "info $value001 $best_ref->{pv}";

                    out_log \%status, "PV< $tmp\n";
                    foreach my $client_ref ( @client ) {
                        if ( $client_ref->{send_pv} ) {
                            print { $client_ref->{sckt} } "$tmp\n";
                        }
                    }
                }

                # 評価値の更新を通知
                change_evaluation_value \%status, $best_value
            }
        }

        # split root if possible
        my ( $score_ref ) = ( grep { not $_->{expanded} } values %score );
        my $dbg_expand     = 0;
        my $dbg_not_expand = 0;
        if ( defined $score_ref
             and not $score_ref->{final}
             and @expanded_moves + 1 < @client
             and $status{split_nodes} < $score_ref->{nodes} ) {
            
            my $move  = $score_ref->{move};
            my $first = 1;
            foreach my $client_ref ( @client ) {

                if ( defined $client_ref->{played_move} ) { next; }

                if ( $first ) {
                    my $pid  = $global_pid + 1;
                    my $line = "pmove $move $pid";
                    out_log \%status, "$client_ref->{id}< $line\n";
                    print { $client_ref->{sckt} } "$line\n";
                    $client_ref->{played_move} = $move;
                    $client_ref->{best_move}   = undef;
                    $client_ref->{final}       = 0;
                    $client_ref->{stable}      = 0;
                    $client_ref->{nodes}       = 0;
                    $client_ref->{value}       = -$score_ref->{value};
                    $client_ref->{pid}         =  $pid;
                    $first                     = 0;
                    $dbg_expand               += 1;
                }
                else {
                    my $pid   = $global_pid + 2;
                    my $moves = join ' ', ( @expanded_moves, $move );
                    my $line  = "ignore $pid $moves";
                    out_log \%status, "$client_ref->{id}< $line\n";
                    print { $client_ref->{sckt} } "$line\n";
                    $client_ref->{best_move} = undef;
                    $client_ref->{final}     = 0;
                    $client_ref->{stable}    = 0;
                    $client_ref->{nodes}     = 0;
                    $client_ref->{value}     = 0;
                    $client_ref->{pid}       =  $pid;
                    $dbg_not_expand         += 1;
                }
            }
            if ( $dbg_expand != 1 or $dbg_not_expand < 1 ) { die; }
            $global_pid += 2;
        }
    }
}


sub sort_clients($$) {
    my ( $status_ref, $clients_ref ) = @_;

#    foreach my $client ( @$clients_ref ) {
#        print "$client->{id} $client->{nthreads}\n";
#    }

    @$clients_ref = sort
        { $b->{nthreads} <=> $a->{nthreads} }
        @$clients_ref;

#    foreach my $client ( @$clients_ref ) {
#        print "$client->{id} $client->{nthreads}\n";
#    }
}


sub get_client_from_key($$) {
    my ( $client_ref, $key_sckt ) = @_;

    my ( $client ) = grep( { $_->{sckt} eq $key_sckt }
                           @$client_ref );

#    print %$client;
    return $client;
}


sub array_remove (&$) {
    my ( $test_block, $arr_ref ) = @_;
    my $sp_start  = 0;
    my $sp_len    = 0;

    for ( my $inx = 0; $inx <= $#$arr_ref; $inx++ ) {
        local $_ = $arr_ref->[$inx];
        next unless $test_block->( $_ );
        if ( $sp_len > 0 && $inx > $sp_start + $sp_len ) {
            splice( @$arr_ref, $sp_start, $sp_len );
            $inx    = $inx - $sp_len;
            $sp_len = 0;
        }
        $sp_start = $inx if ++$sp_len == 1;
    }

    splice( @$arr_ref, $sp_start, $sp_len ) if $sp_len > 0;
}


sub change_evaluation_value($;$) {
    my ( $status_ref, $eval_value ) = @_;

    # 最初に評価値を出力。
    if ($#_ < 1) { $eval_value = $g_eval_value; }

    # 評価値を保存。
    $g_eval_value = $eval_value;
    $g_need_update = 1;
}


sub update_godwhale($) {
    my ( $status_ref ) = @_;
    my $time = time();
    my $update_freq = 3.0; # 秒

    if ( not $g_need_update or
         $time < $g_update_lasttime + $update_freq ) {
        return;
    }

    # 参加者一覧のリストを出力します。
    my $filepath = "$ENV{'HOME'}/.whale_list";
    open FH, ">$filepath";

    # 最初に評価値を出力。
    print FH "$g_eval_value\n";

    # 次に参加者を出力。
    foreach my $c ( @client ) {
        print FH "$c->{id}\n";
    }
    close FH;

    # VoteServerにシグナルを出します。
    my $command = "/bin/ps x | grep VoteServer.exe | grep -v grep | cut -c 1-5 |";
    open PS, $command;
    my $line = <PS>;
    if ( $line ne '' ) {
        #print "kill $line";
        kill 'SIGUSR2', $line;
    }
    close PS;

    $g_update_lasttime = time();
    $g_need_update = 0;
}


sub update_current_info($$) {
    my ( $status_ref, $clients_ref ) = @_;
    my $time = time();
    my $update_freq = 5.0; # 秒

    if ( $time < $g_update_lasttime + $update_freq ) {
        return;
    }

    my %bucket = ();
    foreach my $client_ref ( @$clients_ref ) {
	my $pid = $client_ref->{pid};

	if ( exists $bucket{$pid} ) {
            if ( $bucket{$pid}->{final} ) { next; }

            if ( not $client_ref->{final}
                 and $client_ref->{nodes} <= $bucket{$pid}->{nodes} ) {
                next;
            }
        }

	$bucket{$pid} = $client_ref;
    }

    my $node_sum = 0;
    foreach my $client_ref ( values %bucket ) {
        $node_sum += $client_ref->{nodes};
    }

    my $count = @$clients_ref;
    my $line = "info current $count $node_sum $g_eval_value";
    out_clients $status_ref, $clients_ref, $line;

    $g_update_lasttime = time();
}


sub parse_smsg($$$) {
    my ( $status_ref, $client_ref, $line ) = @_;

    out_log $status_ref, "\nServer> $line\n";

    if ( $line =~ /^\s*$/ ) { return 1; }
    elsif ( $line eq 'idle' ) {
        foreach my $ref ( @$client_ref ) {
            $status_ref->{sent_final} = 1;
            out_log $status_ref, "$ref->{id}< idle\n";
            print { $ref->{sckt} } "idle\n";
        }

        delete $status_ref->{gameinfo};
    }
    elsif ( $line eq 'new' ) {
        next_log $status_ref;
        $status_ref->{sent_final}       = 0;
        $status_ref->{server_send_mmsg} = q{};
        $status_ref->{server_pid}       = 0;
        $global_pid                     = 0;
        @movelist                       = ();

        foreach my $ref ( @$client_ref ) {
            out_log $status_ref, "$ref->{id}< new\n";
            print { $ref->{sckt} } "new\n";
            print { $ref->{sckt} } "init 0\n";
            $ref->{played_move} = undef;
            $ref->{best_move}   = undef;
            $ref->{pv}          = undef;
            $ref->{final}       = 0;
            $ref->{stable}      = 0;
            $ref->{nodes}       = 0;
            $ref->{value}       = 0;
            $ref->{pid}         = 0;
        }
    }
    elsif ( $line =~ /^info / ) {
	out_log $status_ref, "$line\n";

	foreach my $ref ( @$client_ref ) {
	    print { $ref->{sckt} } "$line\n";
        }

	if ( $line =~ /^info gameinfo / ) {
	    $status_ref->{gameinfo} = $line;
	}
    }
    elsif ( $line =~ /^(pmove|move) (\d\d\d\d[A-Z][A-Z]) (\d+)\s*(\d+)?$/ ) {
	my $sec = $4 ? $4 : "0";
        $status_ref->{sent_final}  = 0;
        $status_ref->{server_pid}  = $3;
        $global_pid               += 1;

	if ( $1 eq "move" ) {
	    $status_ref->{ponder_move} = undef;
	    push @movelist, $2;
	}
	else {
	    # PVを組み立てるためにponder_moveが必要
	    $status_ref->{ponder_move} = $2;
	}

        my $pid = $global_pid;
        foreach my $ref ( @$client_ref ) {
            if ( defined $ref->{played_move} and $ref->{played_move} eq $2 ) {
                $pid = $ref->{pid};
                last;
            }
        }

        foreach my $ref ( @$client_ref ) {
            if ( defined $ref->{played_move} and $ref->{played_move} eq $2 ) {
                out_log $status_ref, "$ref->{id} has the same position.\n";
		print { $ref->{sckt} } "movehit $ref->{played_move} $pid $sec\n";
                $ref->{played_move} = undef;
		$ref->{pv}          = undef;
                $ref->{value}       = -$ref->{value};
                $ref->{pid}         = $pid;
            }
            else {
                if ( defined $ref->{played_move} ) {
                    $line= "alter $2 $global_pid $sec";
                }
                else {
                    $line= "$1 $2 $global_pid $sec";
                }
                out_log $status_ref, "$ref->{id}< $line\n";
                print { $ref->{sckt} } "$line\n";
                $ref->{played_move} = undef;
                $ref->{best_move}   = undef;
                $ref->{pv}          = undef;
                $ref->{final}       = 0;
                $ref->{stable}      = 0;
                $ref->{nodes}       = 0;
                $ref->{value}       = 0;
                $ref->{pid}         = $pid;
            }
        }
    }
    elsif ( $line =~ /alter (\d\d\d\d[A-Z][A-Z]) (\d+)\s*(\d+)?$/ ) {
	my $sec = $3 ? $3 : "0";
        $status_ref->{sent_final}  = 0;
        $status_ref->{server_pid}  = $2;
	$status_ref->{ponder_move} = undef;
        $global_pid               += 1;
        push @movelist, $1;

	$line = "alter $1 $2 $sec";

        foreach my $ref ( @$client_ref ) {

            if ( defined $ref->{played_move} ) {
                $line = "retract 2 $1 $global_pid $sec";
            }
            else { $line = "alter $1 $global_pid $sec"; }

            out_log $status_ref, "$ref->{id}< $line\n";
            print { $ref->{sckt} } "$line\n";
            $ref->{played_move} = undef;
            $ref->{best_move}   = undef;
	    $ref->{pv}          = undef;
            $ref->{final}       = 0;
            $ref->{stable}      = 0;
            $ref->{nodes}       = 0;
            $ref->{value}       = 0;
            $ref->{pid}         = $global_pid;
        }
    }
    elsif ( $line =~ /ponderhit (\d\d\d\d[A-Z][A-Z]) (\d+)\s*(\d+)?$/ ) {
	my $sec = $3 ? $3 : "0";
	push @movelist, $1;

	$line = "ponderhit $1 $2 $sec";

	out_log $status_ref, "ALL< $line\n";

	foreach my $ref ( @$client_ref ) {
	    print { $ref->{sckt} } "$line\n";

	    $ref->{pv} = undef;
	}
    }
    else { return 0; }

    return 1;
}


sub parse_cmsg($$$) {
    my ( $status_ref, $client_ref, $line ) = @_;
    my ( $client_pid, $client_move, $client_nodes, $client_value );
    my ( $client_pv );
    
    out_log $status_ref, "$client_ref->{id}> $line\n";
#    print "$line\n";

    if ( $line =~ /^$/ ) {
        print { $client_ref->{sckt} } "\n";
        return 1;
    }

    if( $line =~ /pid=(\d+)/ ) { $client_pid = $1; }

    if ( $line =~ /^login/ ) {
	parse_clogin $status_ref, $client_ref, $line;
        $client_ref->{pid} = $global_pid;

        my $movestr = join ' ', @movelist;
        print { $client_ref->{sckt} } "new\n";
        print { $client_ref->{sckt} } "init $global_pid $movestr\n";

        out_log $status_ref, "LOGIN: $line\n";
        out_log $status_ref, "init $global_pid $movestr\n";

	if ( $status_ref->{gameinfo} ) {
            print { $client_ref->{sckt} } "$status_ref->{gameinfo}\n";
            out_log $status_ref, "$status_ref->{gameinfo}\n";
	}

	if ( $status_ref->{ponder_move} ) {
	    my $tmp = "pmove $status_ref->{ponder_move} $global_pid\n";

	    print { $client_ref->{sckt} } "$tmp\n";
	    out_log $status_ref, "$tmp\n";
	}
    }
    elsif ( not defined $client_pid ) { return 0; }

    if ( defined $client_pid and $client_pid != $client_ref->{pid} ) { return 1; }

    my $client_final  = 0;
    my $client_stable = 0;
    if    ( $line =~ /move=(\d\d\d\d[A-Z][A-Z])/ ) { $client_move = $1 }
    elsif ( $line =~ /move=%TORYO/ )               { $client_move = '%TORYO'; }

    if ( $line =~ /n=(\d+)/ )                          { $client_nodes  = $1 }
    if ( $line =~ /v=([+-]?\d+)/ )                     { $client_value  = $1 }
    if ( $line =~ /pv=(( [+-]?\d\d\d\d[A-Z][A-Z])+)/ ) { $client_pv     = $1 }
    if ( $line =~ /final/ )                            { $client_final  = 1  }
    if ( $line =~ /stable/ )                           { $client_stable = 1  }

    if ( defined $client_move
         and defined $client_nodes
         and defined $client_value ) {
        $client_ref->{best_move} = $client_move;
        $client_ref->{nodes}     = $client_nodes;
        $client_ref->{value}     = $client_value;
        $client_ref->{final}     = $client_final;
        $client_ref->{stable}    = $client_stable;
	$client_ref->{pv}        =
	    ( $status_ref->{ponder_move} ? $status_ref->{ponder_move} : "" ) .
	    ( $client_ref->{played_move} ? " $client_ref->{played_move}" : "" ) .
	    ( $client_pv ? $client_pv : "" );
	return 2;
    }
    elsif ( $client_final ) { $client_ref->{final} = $client_final; }
    elsif ( $client_nodes ) { $client_ref->{nodes} = $client_nodes; }
    else { return 0; }

    return 1;
}

sub parse_clogin($$$) {
    my ( $status_ref, $client_ref, $line ) = @_;

    if ( not 'unknown' eq $client_ref->{id} ) {
        return;
    }

    if ( $line =~ /^login (\w+) (\d+) (\d+)?/ ) {
        $client_ref->{id}       = $1;
        $client_ref->{nthreads} = $2 + 0; # 数値化
	$client_ref->{send_pv}  = $3 ? $3 + 0 : 0;
        $status_ref->{need_client_sort} = 1;
	
        out_log $status_ref,
                "LOGIN: $client_ref->{id} $client_ref->{nthreads}\n";
        $g_need_update = 1;
    }
}
    
# 各クライアント/サーバーのどれかがメッセージを受信するのを待ちます。
sub wait_messages($$$$$$) {

    my ( $status_ref, $listening_sckt, $client_ref, $server_sckt, $server_buf_ref,
        $timeout ) = @_;
    my ( @ready_keys );
    
    # get all handles to wait for messages
    my ( @handles ) = map { $_->{sckt} } @$client_ref;
    if ( defined $listening_sckt ) { push @handles, $listening_sckt; }
    if ( defined $server_sckt )    { push @handles, $server_sckt; }

    my $selector = new IO::Select( @handles );
    
    while (1) {
        @ready_keys
            = grep( { index( $_->{in_buf}, "\n" ) != -1 }
                    @$client_ref );

        if ( defined $server_buf_ref
             and index( $$server_buf_ref, "\n" ) != -1 ) {
            push @ready_keys, 'server socket';
        }
        if ( @ready_keys ) { last; }

        my ( @sckts ) = $selector->can_read( $timeout );
        if ( not @sckts ) { return; }

        foreach my $in ( @sckts ) {

            if ( defined $listening_sckt and $in eq $listening_sckt ) {
                
                # accept the connection for a client
                my $accepted_sckt = $in->accept or die "accept failed: $!\n";
                my ( $port, $iaddr )
                    = unpack_sockaddr_in $accepted_sckt->peername;
                my $str_iaddr = inet_ntoa $iaddr;
                
                $selector->add( $accepted_sckt );
                my %client = (
                    id          => 'unknown',
		    nthreads    => 0,
		    send_pv     => 0,
                    iaddr       => $str_iaddr,
                    port        => $port,
                    in_buf      => q{},
                    sckt        => $accepted_sckt,
                    stable      => 0,
                    final       => 0,
                    played_move => undef,
                    best_move   => undef,
		    pv          => undef,
                    nodes       => 0,
                    value       => 0,
                    pid         => 0 );
                push @$client_ref, \%client;
                
                print $accepted_sckt "idle\n";
                next;
            }
            
            # message arrived from a server
            if ( defined $server_sckt and $in eq $server_sckt ) {
                
                my $in_buf;
                $in->recv( $in_buf, 65536 );
                if ( not $in_buf ) { die 'The server is down.'; }
                $$server_buf_ref .= $in_buf;
                $$server_buf_ref  =~ s/\015?\012/\n/g;
                next;
            }

	    my $client = get_client_from_key $client_ref, $in;

            # message arrived from a client
            my $in_buf;
            $in->recv( $in_buf, 65536 );
            if ( not $in_buf ) {
                my $size = @$client_ref - 1;
                out_log $status_ref, "$client->{id} is down. ($size)\n";

                $selector->remove( $in );
		array_remove { $_->{sckt} eq $in } $client_ref;
                next;
            }

            $client->{in_buf} .= $in_buf;
            $client->{in_buf}  =~ s/\015?\012/\n/g;
        }
    }
    
    return @ready_keys;
}


sub get_line ($) {
    my ( $line_ref ) = @_;

    my $pos = index $$line_ref, "\n";
    if ( $pos == -1 ) { die "Internal error." }
    
    my $line   = substr $$line_ref, 0, 1+$pos;
    $$line_ref = substr $$line_ref, 1+$pos;
    chomp $line;

    return $line;
}


sub out_clients ($$$) {
    my ( $status_ref, $clients_ref, $line ) = @_;

    out_log $status_ref, "ALL< $line\n";
    foreach my $client ( @$clients_ref ) {
        print { $client->{sckt} } "$line\n";
    }
}


sub next_log ($) {
    my ( $status_ref ) = @_;

    if ( defined $status_ref->{fh_log} ) {
        $status_ref->{fh_log}->close or die "$!";
    }

    my ( $sec, $min, $hour, $mday, $mon, $year ) = localtime( time );
    $year -= 100;
    $mon  += 1;
    my $dirname = sprintf "parallel_%02s%02s", $year, $mon;
    if ( not -d $dirname ) { mkdir $dirname or die "$!"; }

    my $basename = sprintf "%02s%02s%02s%02s", $mday, $hour, $min, $sec;
    my $filename = File::Spec->catfile( $dirname, $basename );

    open $status_ref->{fh_log}, ">${filename}.log" or die "$!";
    $status_ref->{fh_log}->autoflush(1);
}


sub out_log ($$) {
    my ( $status_ref, $msg ) = @_;

    if ( defined $status_ref->{fh_log} ) {
        print { $status_ref->{fh_log} } $msg;
    }
    else { print $msg; }
}
