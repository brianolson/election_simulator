#!/usr/bin/perl -w

sub stdev(@) {
  my $i;
  my $avg = 0.0;
  my @v = @_;
  for ( $i = 0; $i <= $#v; $i++ ) {
    $avg += $v[$i];
  }
  $avg /= ($#v + 1);
  my $var = 0.0;
  for ( $i = 0; $i <= $#v; $i++ ) {
    my $d;
    $d = $avg - $v[$i];
    $var += ( $d * $d );
  }
  return sqrt( $var / $#v );
}

sub gini(@) {
  my $i;
  my $j;
  my $sum = 0.0;
  my $gs = 0.0;
  my @v = @_;
  for ( $i = 0; $i <= $#v; $i++ ) {
    $sum += $v[$i];
    for ( $j = $i + 1; $j <= $#v; $j++ ) {
      $gs += abs( $v[$i] - $v[$j] );
    }
  }
#  printf STDERR "gini %f = %f / ( %f * %f )\n", ($gs / (($#v + 1) * $sum)), $gs, ($#v + 1), $sum;
  return $gs / (($#v + 1) * $sum);
}

# O( n log n ) version. sort then linear pass.
# G = abs( 1 - sum{1..n}{(X_k - X_k-1)(Y_k + Y_k-1)} )
sub gini2(@) {
	my $i;
	my $j;
	my $sum = 0.0;
	my $gs = 0.0;
	my @v = sort @_;
	my $prevy = 0;
	my $ps = 0;
	for ( $i = 0; $i <= $#v; $i++ ) {
		$sum += $v[$i];
#		$xk = $i / $#v;
#		$xkm1 = ($i-1) / $#v;
		$yk = $sum; # / $endsum
		$ykm1 = $ps;
#		$gs += ($xk - $xkm1) * ($yk + $ykm1);
		$gs += (1 / ($#v + 1)) * ($yk + $ykm1);
		$ps = $sum;
	}
	$gs = $gs / $sum;
	return abs( 1 - $gs );
}

@data = ( 
[ 5, 5, 5, 5 ],
[ 5, 5, 5, 6 ],
[1, 2, 3, 4],
[1,1,1,9],
[1,1,1,999],
[1,1,1,1,1,1,1,1,1,999],
[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,999],
);

foreach $x ( @data ) {
  my @xa = @{$x};
  $std = stdev( @xa );
  print "data: " . join( ", ", @xa ) . "\n";
  print "std: $std\n";
  print "gini: " . gini(@xa) . "\n";
  print "gin2: " . gini2(@xa) . "\n";
  print "\n";
}
