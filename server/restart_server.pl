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

# subroutines
sub load_csafile();
sub get_move($);
sub send_nextmove($$);
sub wait_messages($$$);
sub parse_smsg($$);
sub get_line ($);
sub next_log ($);
sub out_log ($$);

my $server_sckt;
my $move_count = 0;
my @move_list  = load_csafile();

{
    # defaults of command-line options
    my ( %status ) = ( server_host      => 'localhost',
		       server_port      => 4082,
		       server_id        => 'Parallel2',
		       server_buf       => q{},
		       server_pid       => 0,
		       fh_log           => undef );


    # parse command-line options
    GetOptions( \%status,
		'server_host=s',
		'server_port=i',
		'server_id=s' ) or die "$!";

    $server_sckt
	= new IO::Socket::INET( PeerAddr => $status{server_host},
				PeerPort => $status{server_port},
				Proto    => 'tcp' );
    if ( not $server_sckt ) { die "Cannot connect to the majority server."; }

    print $server_sckt "$status{server_id} 100.0 final confident\n";

    # main loop
    my $server_buf = q{};
    while ( 1 ) {

	my $have_message = wait_messages( \%status, \$server_buf, timeout );
	my $time = time();
	if ( $have_message ) {
	    my $line = get_line \$server_buf;
	    if ( not parse_smsg \%status, $line ) {
		die "MESSAGE FROM SERVER: $line";
	    }
	}
    }
}


# 棋譜ファイルを開きます。
sub load_csafile() {
    my $filepath = "kifu.csa";
    my @move_list = ();

    open FH, "<$filepath";
    while (1) {
	my $line = <FH>;
	if ( not $line ) {
	    last;
	}

	if ( $line =~ /(\+|\-)(\d\d\d\d[A-Z][A-Z])\,T(\d+)$/ ) {
	    push @move_list, $2;
#	    print "$2\n";
	}
    }
    close FH;

    return @move_list;
}


sub get_move($) {
    my ( $count ) = @_;

    if ( $count < 0 || $count >= @move_list ) {
	return "";
    }

    return $move_list[$count];
}


sub parse_smsg($$) {
    my ( $status_ref, $line ) = @_;
		
    out_log $status_ref, "\nServer> $line\n";

    if ( $line =~ /^\s*$/ ) { return 1; }
    elsif ( $line eq 'idle' ) {
    }
    elsif ( $line eq 'new' ) {
	next_log $status_ref;
	$status_ref->{server_pid} = 0;
	$move_count               = 0;
    }
    elsif ( $line =~ /move (\d\d\d\d[A-Z][A-Z]) (\d+)$/ ) {
	# move では相手の予想手と実際に指した手の両方が送られてきます。
	$status_ref->{server_pid} = $2;

	if ( $move_count >= 0 ) {
	    $move_count += 1;

	    send_nextmove($status_ref, $1);
	}
    }
    elsif ( $line =~ /alter (\d\d\d\d[A-Z][A-Z]) (\d+)$/ ) {
	$status_ref->{server_pid}  = $2;

	send_nextmove($status_ref, $1);
    }
    else { return 0; }

    return 1;
}


# 予想手は返さないので、相手の手番の後にのみ手を送ります。
# (番号は0から初めるので、CPUが後手なら奇数時に送ります)
sub send_nextmove($$) {
    my ( $status_ref, $move ) = @_;

    if ( $move_count >= 0 ) { #and $move_count % 2 == 1 ) {
#	if ( $move ne get_move($move_count - 1) ) {
#	    $move_count = -1;
#	    return;
#	}

	my $next_move = get_move($move_count);
	if ( $next_move eq "" ) {
	    return;
	}

	my $mmsg =( "pid=$status_ref->{server_pid} move=$next_move "
		    . "v=0e n=0 confident final" );
	out_log $status_ref, "Server< $mmsg\n";
	print $server_sckt "$mmsg\n";
    }
}


# サーバーがメッセージを受信するのを待ちます。
sub wait_messages($$$) {
    my ( $status_ref, $server_buf_ref, $timeout ) = @_;

    if ( defined $server_buf_ref
	 and index( $$server_buf_ref, "\n" ) != -1 ) {
	return 1;
    }
    
    # get all handles to wait for messages
    my $selector = new IO::Select( $server_sckt );
    my ( @sckts ) = $selector->can_read( $timeout );
    if ( not @sckts ) { return 0; }

    my $in_buf;
    $server_sckt->recv( $in_buf, 65536 );
    if ( not $in_buf ) { die 'The server is down.'; }
    $$server_buf_ref .= $in_buf;
    $$server_buf_ref  =~ s/\015?\012/\n/g;
    return 1;
}


sub get_line ($) {
    my ( $line_ref ) = @_;

    my $pos = index $$line_ref, "\n";
    if ( $pos == -1 ) { die 'Internal error' }
	    
    my $line   = substr $$line_ref, 0, 1+$pos;
    $$line_ref = substr $$line_ref, 1+$pos;
    chomp $line;

    return $line;
}


sub next_log ($) {
    my ( $status_ref ) = @_;

    if ( defined $status_ref->{fh_log} ) {
	$status_ref->{fh_log}->close or die "$!";
    }

    my ( $sec, $min, $hour, $mday, $mon, $year ) = localtime( time );
    $year -= 100;
    $mon  += 1;
    my $dirname = sprintf "restart_%02s%02s", $year, $mon;
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
