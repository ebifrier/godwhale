#!/usr/bin/perl -w

# --- hash keys ---
# client: in_buf, id, sckt, final_comm
# client (worker): work_key, cancellation
# client (client): moves, signatures, work_keys, md5, child_stack, best_move
# - id: unknown, woker, client
#
# child: md5, move
#
# work_queue: work_type, moves, child_move, rank
# - work_type: dfpn, legal moves, 
#
# work_result: time, result
# -----------------

# load perl modules
use strict;

use Digest::MD5 qw( md5_hex );
use IO::Socket;
use IO::Select;
use Time::HiRes qw( time );
use Getopt::Long;
use Scalar::Util qw( looks_like_number );

# autoflush STDOUT
$|=1;

# constants
sub timeout           () {  10.0 }
sub keepalive_time    () { 180.0 }
sub forget_check_time () { 60.0 * 60.0 *  1.0 } # one hour
sub forget_time       () { 60.0 * 60.0 * 12.0 } # 12 hours
sub worker_init_str   () { <<'INIT_STR_END';
stdout off
newlog off
ponder off
dfpn hash 24
INIT_STR_END
}

# subroutines
sub wait_messages    ($$$$);
sub parse_msg        ($$);
sub parse_worker_msg ($$$$);
sub parse_client_msg ($$$);
sub moves2cmd        ($);

{
    my ( %client, %work_result );
    my $time     = time;
    my $str_time = localtime $time;
    my $forget_previous_check_time = time;

    print "DFPN server ($str_time)\n";

    # defaults of command-line options
    my ( %cmd_option ) = ( client_port => 4083 );

    GetOptions( \%cmd_option, 'client_port=i' ) or die "$!";

    # create a listening socket for my clients.
    my $listening_sckt
	= new IO::Socket::INET( LocalPort => $cmd_option{client_port},
				Listen    => SOMAXCONN,
				Proto     => 'tcp',
				ReuseAddr => 1 )
	or die "Can't create a listening socket: $!\n";

    # main loop
    while ( 1 ) {
	my ( @ready_keys ) = wait_messages( $listening_sckt, \%client,
					    \%work_result,
					    \$forget_previous_check_time );

	foreach my $key ( @ready_keys ) {

	    my $ref = $client{$key};

	    # get one line from buffer
	    my $pos = index $ref->{in_buf}, "\n";
	    if ( $pos == -1 ) { die 'Internal error' }
	    
	    my $line       = substr $ref->{in_buf}, 0, 1+$pos;
	    $ref->{in_buf} = substr $ref->{in_buf}, 1+$pos;
	    chomp $line;


	    if ( $ref->{id} eq 'unknown' ) {

		if ( not parse_msg $line, $ref ) {
		    print( "parse error: $line"
			   . " ($ref->{iaddr}:$ref->{port})\n" );
		}
	    }
	    elsif ( $ref->{id} eq 'client' ) {


		if ( not parse_client_msg( $line, $key, \%client ) ) {
		    print( "client parse error: $line"
			   . " (client $ref->{iaddr}:$ref->{port})\n" );
		}
		
	    }
	    elsif ( $ref->{id} eq 'worker' ) {

		if ( not parse_worker_msg( $line, $key, \%client,
					   \%work_result ) ) {
		    print( "worker parse error: $line"
			   . " (worker $ref->{iaddr}:$ref->{port})\n" );
		}
		
	    }
	    else { die 'Internal error' }
	}


	# find works from clients, and send those if the works are done.
	my ( %work_queue ) = ();
	foreach my $client_key ( keys %client ) {

	    my $client_ref = $client{$client_key};
	    if ( not $client_ref->{id} eq 'client' ) { next; }
	    if ( not exists $client_ref->{moves} )   { next; }

	    my $work_key;

	    # dfpn search at root node
	    $work_key = $client_ref->{md5} . 'DFPN';
	    if ( not grep { $work_key eq $_ } @{$client_ref->{work_keys}} ) {

		if ( exists $work_result{$work_key} ) {
		
		    push @{$client_ref->{work_keys}}, $work_key;
		
		    my $signature = $client_ref->{signatures}->[-1];
		    print( { $client_ref->{sckt} }
			   "$signature $work_result{$work_key}->{result}\n" );
		}
		else {
		    $work_queue{$work_key}={ work_type => 'DFPN',
					     moves     => $client_ref->{moves},
					     rank      => 1 };
		}

	    }
	    
	    $work_key = $client_ref->{md5} . 'MOVE';
	    if ( not grep { $work_key eq $_ } @{$client_ref->{work_keys}} ) {
		if ( exists $work_result{$work_key} ) {
		
		    push @{$client_ref->{work_keys}}, $work_key;
		    my ( @moves ) = split( ' ',
					   $work_result{$work_key}->{result} );
		    $client_ref->{child_stack} = [];
		    foreach my $move ( @moves ) {
			my $md5 = md5_hex @{$client_ref->{moves}}, $move;
			push @{$client_ref->{child_stack}}, { md5  => $md5,
							      move => $move };
		    }
		}
		else {
		    $work_queue{$work_key}={ work_type => 'MOVE',
					     moves     => $client_ref->{moves},
					     rank      => 2 };
		}
	    }


	    # dfpn searchs at child nodes
	    my ( @children ) = ();
	    foreach my $child_ref ( @{$client_ref->{child_stack}} ) {
		my $work_key   = $child_ref->{md5} . 'DFPN';
		my $child_move = $child_ref->{move};
		if ( exists $work_result{$work_key} ) {
		    
		    push @{$client_ref->{work_keys}}, $work_key;

		    my $signature = $client_ref->{signatures}->[-1];
		    print( { $client_ref->{sckt} }
			   "$signature $child_move "
			   . "$work_result{$work_key}->{result}\n" );
		}
		else {
		    push @children, $child_ref;
		    if ( not exists $work_queue{$work_key} ) {

			my $rank;
			if ( defined $client_ref->{best_move}
			     and  $client_ref->{best_move} eq $child_move ) {
			    $rank = 3;
			} else { $rank = 4; }

			$work_queue{$work_key}
			= { work_type  => 'DFPN',
			    moves      => $client_ref->{moves},
			    child_move => $child_move,
			    rank       => $rank };
		    }
		}
	    }
	    $client_ref->{child_stack} = \@children;
	}


	# find free and busy workers from %client
	# cancel work of busy workers.
	my ( @free_worker_keys ) = ();
	foreach my $worker_key ( keys %client ) {
	    
	    my $worker_ref = $client{$worker_key};

	    if ( not $worker_ref->{id} eq 'worker' ) { next; }

	    if ( defined $worker_ref->{work_key} ) {

		my $work_key = $worker_ref->{work_key};
		if ( not $worker_ref->{cancellation}
		     and not exists $work_queue{$work_key} ) {
		    
		    $worker_ref->{cancellation} = 1;
		    print { $worker_ref->{sckt} } "s\n";
		}
	    }
	    else { push @free_worker_keys, $worker_key; }
	}


	# find work assigned already
	my ( @assigned_work_keys );
	foreach my $client_key ( keys %client ) {
	    
	    my $worker_ref = $client{$client_key};
	    if ( not $worker_ref->{id} eq 'worker' ) { next; }
	    if ( defined $worker_ref->{work_key}
		 and $worker_ref->{cancellation} == 0 ) {
		push @assigned_work_keys, $worker_ref->{work_key};
	    }
	}


	# sort work_key in order of rank
	my ( @work_keys ) = sort( { $work_queue{$a}->{rank}
				    <=> $work_queue{$b}->{rank} }
				  keys %work_queue );


	# assign work to free workers
	foreach my $work_key ( @work_keys ) {

	    if ( grep { $work_key eq $_ } @assigned_work_keys ) { next; }

	    my $worker_key = shift @free_worker_keys;
	    if ( not defined $worker_key ) { last; }

	    my $worker_ref              = $client{$worker_key};
	    $worker_ref->{work_key}     = $work_key;
	    $worker_ref->{cancellation} = 0;

	    
	    my ( @moves ) = @{$work_queue{$work_key}->{moves}};
	    if ( exists $work_queue{$work_key}->{child_move} ) {
		push @moves, $work_queue{$work_key}->{child_move};
	    }
	    my $cmd = moves2cmd \@moves;
	    print { $worker_ref->{sckt} } "$cmd\n";

	    if ( $work_queue{$work_key}->{work_type} eq 'DFPN' ) {
		print { $worker_ref->{sckt} } "dfpn go\n";
	    }
	    elsif ( $work_queue{$work_key}->{work_type} eq 'MOVE' ) {
		print { $worker_ref->{sckt} } "outmove\n";
	    }
	}
    }
}


sub parse_msg ($$) {
    my ( $msg, $client_ref ) = @_;
    my ( $str_time, $str_iaddr, $port );

    if    ( $msg =~ /^ping$/ or $msg =~ /^pong$/ ) { ; }
    elsif ( $msg =~ /^Worker: (.+)/ ) {
	$client_ref->{id}              = 'worker';
	$client_ref->{work_key}        = undef;
	$client_ref->{cancellation}    = 0;
	print {$client_ref->{sckt}} worker_init_str;

	$str_iaddr = $client_ref->{iaddr};
	$port      = $client_ref->{port};
	$str_time  = localtime $client_ref->{final_comm};
	print "A worker login from $str_iaddr:$port ($str_time).\n";
    }
    elsif ( $msg =~ /^Client: (.+)/ ) {
	$client_ref->{id}        = 'client';
	$client_ref->{work_keys} = [];

	$str_iaddr = $client_ref->{iaddr};
	$port      = $client_ref->{port};
	$str_time  = localtime $client_ref->{final_comm};
	print "A client login from $str_iaddr:$port ($str_time).\n";
    }
    else { return 0; }
}


sub parse_worker_msg ($$$$) {
    my ( $msg, $key, $client_ref, $work_result_ref ) = @_;
    
    my $ref = $client_ref->{$key};
    if ( not $ref->{id} eq 'worker' ) { die 'Internal error' }

    if    ( $msg =~ /^$/ or $msg =~ /^pong$/ ) { ; }
    elsif ( $msg =~ /^quit$/ ) {

	$ref->{sckt}->close;
	delete $client_ref->{$key};
    }
    elsif ( $msg =~ /^UNSOLVED -105$/ ) {

	if ( not defined $ref->{work_key} ) { return 0; }
	my $work_key         = $ref->{work_key};
	$ref->{work_key}     = undef;
	$ref->{cancellation} = 0;
    }
    elsif ( $msg =~ /^NO LEGAL MOVE$/ ) {

	if ( not defined $ref->{work_key} ) { return 0; }
	my $work_key         = $ref->{work_key};
	$ref->{work_key}     = undef;
	$ref->{cancellation} = 0;
	$work_result_ref->{$work_key} = { result => q{},
					  time   => $ref->{final_comm} };
    }
    elsif ( $msg =~ /^WIN \d\d\d\d[A-Z][A-Z]$/
	    or $msg =~ /^(\d\d\d\d[A-Z][A-Z]\s?)+$/
	    or $msg =~ /^LOSE$/
	    or $msg =~ /^UNSOLVED/ ) {

	if ( not defined $ref->{work_key} ) { return 0; }
	my $work_key         = $ref->{work_key};
	$ref->{work_key}     = undef;
	$ref->{cancellation} = 0;
	$work_result_ref->{$work_key} = { result => $msg,
					  time   => $ref->{final_comm} };
    }
    else { return 0; }

    return 1;
}


sub parse_client_msg ($$$) {

    my ( $msg, $key, $client_ref ) = @_;

    my $ref = $client_ref->{$key};
    if ( not $ref->{id} eq 'client' ) { die 'Internal error' }

    if    ( $msg =~ /^ping$/ or $msg =~ /^pong$/ ) { ; }
    elsif ( $msg =~ /^new (.+)$/ ) {

	$ref->{moves}       = ['new'];
	$ref->{signatures}  = [$1];
	$ref->{work_keys}   = [];
	$ref->{child_stack} = [];
	$ref->{best_move}   = undef;
	$ref->{md5}         = md5_hex @{ $ref->{moves} };
    }
    elsif ( $msg =~ /^(\d\d\d\d[A-Z][A-Z]) (.+)$/ ) {
	
	if ( not exists $ref->{moves}
	     or not defined $ref->{moves}->[0]
	     or not $ref->{moves}->[0] eq 'new' ) { return 0; }
	
	push @{$ref->{moves}},      $1;
	push @{$ref->{signatures}}, $2;
	$ref->{best_move}   = undef;
	$ref->{work_keys}   = [];
	$ref->{child_stack} = [];
	$ref->{md5}         = md5_hex @{ $ref->{moves} };
    }
    elsif ( $msg =~ /^unmake$/ ) {
	
	if ( not exists $ref->{moves}
	     or @{$ref->{moves}} < 2
	     or not $ref->{moves}->[0] eq 'new' ) { return 0; }
	
	pop @{$ref->{moves}};
	pop @{$ref->{signatures}};
	$ref->{work_keys}   = [];
	$ref->{child_stack} = [];
	$ref->{md5}         = md5_hex @{ $ref->{moves} };
    }
    elsif ( $msg =~ /^BEST MOVE (\d\d\d\d[A-Z][A-Z])$/) {
	$ref->{best_move}   = $1;
    }
    elsif ( $msg =~ /^quit$/ ) {

	$ref->{sckt}->close;
	delete $client_ref->{$key};
    }
    else { return 0; }

    return 1;
}

    
sub wait_messages($$$$) {

    my ( $listening_sckt, $client_ref, $work_result_ref,
	 $previous_check_time_ref ) = @_;
    my ( @ready_keys );
    
    # get all handles to wait for messages
    my ( @handles ) = ( $listening_sckt,
			map { $client_ref->{$_}->{sckt} } keys %$client_ref );
    my $selector = new IO::Select( @handles );
    
    while (1) {
	@ready_keys
	    = grep( { index( $client_ref->{$_}->{in_buf}, "\n" ) != -1 }
		    keys %$client_ref );
	
	if ( @ready_keys ) { last; }

	my ( @ready_handles ) = $selector->can_read( timeout );
	my $time              = time;

	foreach my $in ( @ready_handles ) {
	    
	    if ( $in == $listening_sckt ) {
		
		# accept the connection for a client
		my $accepted_sckt = $in->accept or die "accept failed: $!\n";
		my ( $port, $iaddr )
		    = unpack_sockaddr_in $accepted_sckt->peername;
		my $str_iaddr = inet_ntoa $iaddr;
		
		$selector->add( $accepted_sckt );
		$client_ref->{ $accepted_sckt }	= {
		    id          => 'unknown',
		    iaddr       => $str_iaddr,
		    port        => $port,
		    final_comm  => $time,
		    in_buf      => q{},
		    sckt        => $accepted_sckt };
		
		next;
	    }
	    
	    # message arrived from a client
	    my $in_buf;
	    $in->recv( $in_buf, 65536 );
	    if ( $in_buf ) {
		$client_ref->{$in}->{final_comm}  = $time;
		$client_ref->{$in}->{in_buf}     .= $in_buf;
		$client_ref->{$in}->{in_buf}      =~ s/\015?\012/\n/g;
	    }
	    else {
		# A client is down.
		print( 'WARNING: A connection from'
		       . " $client_ref->{$in}->{iaddr}"
		       . ":$client_ref->{$in}->{port}"
		       . " ($client_ref->{$in}->{id}) is down.\n" );

		$client_ref->{$in}->{final_comm} = $time;
		$client_ref->{$in}->{in_buf}     = "quit\n";
		$selector->remove( $in );
	    }
	}

	# forget old results
	if ( not @ready_handles
	     and ( $$previous_check_time_ref + forget_check_time < $time ) ) {
	    my $time     = time;
	    my $str_time = localtime $time;
	    my $count0   = 0;
	    my $count1   = 0;

	    print "- Cleen up table ($str_time)\n";
	    $$previous_check_time_ref = $time;
	    foreach my $work_key ( keys %$work_result_ref ) {
		$count1 += 1;
		my $work_ref = $work_result_ref->{$work_key};
		
		if ( $work_ref->{time} + forget_time < $time ) {
		    $count0 += 1;
		    delete $work_result_ref->{ $work_key };
		}
	    }
	    $time     = time;
	    $str_time = localtime $time;
	    print "  Done ($str_time).\n";
	    print "  $count0 / $count1\n";
	}

	# keep alive
	foreach my $ref ( values %$client_ref ) {
	    
	    if ( $time - keepalive_time < $ref->{final_comm} ) { next; }
	    
	    print { $ref->{sckt} } "ping\n";
	    $ref->{final_comm} = $time;
	}
    }
    
    return @ready_keys;
}


sub moves2cmd ($) {
    my ( $moves_ref ) = @_;
    my ( $i, $n, $c, $str );
    
    $str = "new";
    $n = @{ $moves_ref };
    
    for ( $i = 1, $c = 0; $i < $n; $i += 1 ) {
	
	if ( $c == 0 ) { $str .= "\nmove"; }
	$str .= " $moves_ref->[$i]";
	if ( $c == 7 ) { $c  = 0; }
	else           { $c += 1; }
    }

    return $str;
}
