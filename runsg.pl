#!/usr/bin/perl -w

@methods = qw(Max OneVote IRV IRNR Condorcet Rated Borda ApprovalNoInfo ApprovalWithPoll VoteForAndAgainst MaximizedRatings Closest);
%options = ( "MaximizedRatings" => "Rated --opt maximize",
"Closest" => "Max -v 10 -Z 0.000001",
"CombinatoricCondorcet" => "Condorcet --combine",
"CombinatoricIRNR" => "IRNR --combine",
"IRNRPl2" => "IRNRP --opt l2norm",
);
$world = "-px 400 -py 400 -n 4 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 10000 --threads 2";
@candsets = (
["threecorners","-c 1,1 -c -1,1 -c 0,-1"],
["fourcorners","-c 1,1 -c -1,1 -c -1,-1 -c 1,-1"],
["3a","-c -0.86,-0.66 -c -0.02,-0.98 -c -0.18,-0.96"],
["3b","-c 0.86,-0.02 -c 0.58,-0.16 -c -0.46,-0.10"],
["3c","-c 0.08,-0.06 -c 0.54,0.28 -c -0.74,-0.80"],
["4a","-c -0.76,-0.44 -c 0.70,0.40 -c -0.22,-0.44 -c 0.94,-0.72"],
["4b","-c -0.52,-0.54 -c -0.62,0.24 -c -0.92,0.28 -c 0.70,0.10"],
["4c","-c -0.20,0.14 -c -0.68,0.08 -c -0.90,0.24 -c 0.82,0.40"],
);

$candz = undef;

if ( -f "candlist" ) {
  open FIN, '<', "candlist";
  @candsets = ();
  while ($line = <FIN>) {
    $line =~ s/[\r\n]*$//g;
    @a = split( /\t/, $line );
    ($z) = $a[1] =~ /(-Z [0-9.]+)/;
    if ( defined $z ) {
      if ( defined $candz ) {
        if ( $z eq $candz ) {
        } else {
          $candz = "bad";
        }
      } else {
        $candz = $z;
      }
    }
    push @candsets, [@a];
  }
}
if ( ! defined $candz ) {
  $candz = "";
}
$noreplace = 0;

# true if a newerthan b, or b doesn't exist
sub newerthan ($$) {
  my $a = shift;
  my $b = shift;
  my @sa;
  my @sb;
  my $mtimea;
  my $mtimeb;
  @sa = stat($a);
  if ( ! @sa ) {
#    print "$a doesn't exist\n";
    return 0;
  }
  @sb = stat($b);
  if ( ! @sb ) {
#    print "$b doesn't exist\n";
    return 1;
  } elsif ( $noreplace ) {
# b does exist, don't replace it.
    return 0;
  }
  $mtimea = $sa[9];
  $mtimeb = $sb[9];
  if ( ! defined $mtimea ) {
    return 0;
  }
  if ( ! defined $mtimeb ) {
    return 1;
  }
  return $mtimea > $mtimeb;
};

$doit = 1;
$quiet = 0;
$html = 0;
$doplanes = -f "doplanes";

# defaults from spacegraph.cpp  PlaneSim::PlaneSim()
$px = 500;
$py = 500;
$sigma = 0.5;
$minx = -2.0;
$miny = -2.0;
$maxx = 2.0;
$maxy = 2.0;
$electionsPerPixel = 10;
$voters = 1000;

if ( -f "method_list" ) {
	open FIN, '<', "method_list";
	@l = <FIN>;
	close FIN;
	$d = join("",@l);
	@methods = split( /\s+/, $d );
#	print "read method_list: \"" . join("\", \"", @methods ) . "\"\n";
#	exit 1;
}
if ( -f "global_options" ) {
	open FIN, '<', "global_options";
	@l = <FIN>;
	close FIN;
	$world = join("",@l);
	$world =~ s/[\r\n]+/ /g;
	$world =~ s/^\s+//g;
	$world =~ s/\s+$//g;
#	print "read global_options: \"$world\"\n";
#	exit 1;
}

while ( $arg = shift ) {
  if ( $arg eq "-n" ) {
    $doit = 0;
  } elsif ( $arg eq "-q" ) {
    $quiet = 1;
  } elsif ( $arg eq "-u" || $arg eq "--noreplace" ) {
    $noreplace = 1;
  } elsif ( ($arg eq "--html") || ($arg eq "-html") ) {
	  $html = 1;
  } elsif ( ($arg eq "--methods") || ($arg eq "-methods") ) {
	  my $targ = shift;
	  @methods = split( /\s+/, $targ );
  } else {
    die "bogus arg \"$arg\"\n";
  }
}

@warg = split( /\s+/, $world );
while ( $arg = shift @warg ) {
	if ( $arg eq "-px" ) {
		$px = shift @warg;
	} elsif ( $arg eq "-py" ) {
		$py = shift @warg;
	} elsif ( $arg eq "-miny" ) {
		$miny = shift @warg;
	} elsif ( $arg eq "-maxy" ) {
		$maxy = shift @warg;
	} elsif ( $arg eq "-minx" ) {
		$minx = shift @warg;
	} elsif ( $arg eq "-maxx" ) {
		$maxx = shift @warg;
	} elsif ( $arg eq "-n" ) {
		$electionsPerPixel = shift @warg;
	} elsif ( $arg eq "-v" ) {
		$voters = shift @warg;
	} elsif ( $arg eq "-Z" ) {
		$sigma = shift @warg;
	} else {
# ignore
	}
}

$numToDo = 0;

foreach $cs ( @candsets ) {
  foreach $m ( @methods ) {
    my $dest = $cs->[0] . "_" . $m . ".png";
    if ( newerthan( "spacegraph", $dest ) ) {
      $numToDo++;
    }
  }
}

if ( ! $quiet ) {
	print <<EOF;
# ToDo: $numToDo
EOF
}
$doing = 0;

foreach $cs ( @candsets ) {
  foreach $m ( @methods ) {
    my $dest = $cs->[0] . "_" . $m . ".png";
    my $planeopt = "";
    if ($doplanes) {
      $planeopt = " --planes=" . $cs->[0] . "_" . $m . "_";
    }
    if ( newerthan( "spacegraph", $dest ) ) {
      $doing++;
      my $opt;
      if ( $opt = $options{$m} ) {
	# go with it
      } else {
	$opt = $m;
      }
      my $cmd = "./spacegraph $world $cs->[1] --method $opt -o $dest" . $planeopt;
      if ( ! $quiet ) {
        print <<EOF;
# item: $doing/$numToDo
$cmd
EOF
      }
      if ( $doit ) {
	system $cmd;
      }
	  while ( -f "pause" ) {
		sleep(1);
	  }
	  if ( -f "stop" ) {
		unlink "stop";
		exit 0;
	  }
    }
  }
}

if ( (! -f "pop.png") && ($candz ne "bad") ) {
  my $cmd = "./spacegraph $world $candz -tg pop.png";
  if ( ! $quiet ) {
    print <<EOF;
# pop.png
$cmd
EOF
  }
  if ( $doit ) {
    system $cmd;
  }
}

$cols = 2;
if ( $html ) {
	print <<EOF;
<html><head><title>Election Methods In Space!</title>
<script>
EOF
print "var methods = new Array(\"" . join("\",\"", @methods) . "\");\n";
print "var methodEnable = new Array(\"" . join("\",\"", map { "1" } @methods) . "\");\n";
print "var csn = new Array(\"" . join("\",\"", map { $_->[0] } @candsets ) . "\");\n";
print <<EOF;
var cols = 2;
function debug(str) {//debug
    dbp = document.getElementById( "debug" );//debug
    if ( dbp ) {//debug
		dbp.innerHTML = dbp.innerHTML + str;//debug
    }//debug
}//debug
function updateshows( changed ) {
	colsbox = document.getElementById("colsbox");
	if ( colsbox ) {
		tcols = colsbox.value;
		if ( tcols ) {
			cols = tcols;
		}
	}
	for ( i = 0; i < methods.length; i++ ) {
		if ( methods[i] == changed ) {
			methodEnable[i] = (methodEnable[i] == 0) ? 1 : 0;
			break;
		}
	}
	for ( ci = 0; ci < csn.length; ci++ ) {
		tt = document.getElementById( csn[ci] );
		if ( ! tt ) {
			debug("no id:"+csn[ci]+"<br>");
			continue;
		} else {
			//debug("got csn["+ci+"] "+csn[ci]+"<br>");
		}
		while ( tt.rows.length > 0 ) {
			tt.deleteRow(tt.rows.length - 1);
		}
		row = 0;
		col = 0;
		for ( i = 0; i < methods.length; i++ ) {
			if ( methodEnable[i] == 1 ) {
				if ( col == 0 ) {
					tt.insertRow( row );
					tr = tt.rows[row];
					j = i;
					tc = 0;
					while ( (tc < cols) && (j < methods.length) ) {
						if ( methodEnable[j] == 1 ) {
							tr.insertCell( tc );
							tr.cells[tc].innerHTML = '<b>' + methods[j] + '</b>';
							tc++;
							j++;
						} else {
							j++;
						}
					}
					row++;
					tt.insertRow( row );
				}
				tr = tt.rows[row];
				if ( ! tr ) {
					debug("missing row " + row + " of tt<br>");
				}
				tr.insertCell( col );
				tr.cells[col].innerHTML = '<img src="' + csn[ci] + '_' + methods[i] + '.png" width="$px" height="$py" alt="' + methods[i] + '">';
				col++;
				if ( col == cols ) {
					col = 0;
					row++;
				}
			}
		}
	}
}
</script>
</head><body>
<div id="debug"></div>
<div style="float: right; border-width: 1px; border-color: #000000; border-style: solid; margin: 1ex;">
<table><tr valign="top"><td>Columns:<br><input type="text" id="colsbox" name="colsbox" value="2" size="3" onchange="updateshows('');"></td><td>
EOF
for ( $mi = 0; $mi <= $#methods; $mi++ ) {
	$m = $methods[$mi];
	print <<EOF;
<input type="checkbox" onchange="updateshows('$m');" checked> $m<br>
EOF
	if ( $mi == $#methods / 2 ) {
		print "</td><td>";
	}
}
print <<EOF;
</tr></table></div>
EOF
foreach $hname ( <HEADER*.html> ) {
	open FIN, '<', $hname;
	while ( $line = <FIN> ) {
		print $line;
	}
	close FIN;
}
print <<EOF;
<p>Global arguments: <tt>$world</tt></p>
<p>X range: $minx to $maxx<br>
Y range: $miny to $maxy<br>
$electionsPerPixel elections per pixel<br>
$voters Voters with a Gaussian Distribution sigma of $sigma</p>
<hr>
EOF
	foreach $cs ( @candsets ) {
		print <<EOF;
<h2>$cs->[0]</h2>
<p>Run with arguments: <tt>$cs->[1]</tt></p>
<table border="1" id="$cs->[0]">
EOF
		$i = 0;
		for ( $mi = 0; $mi <= $#methods; $mi++ ) {
			$m = $methods[$mi];
			my $dest = $cs->[0] . "_" . $m . ".png";
			if ( $i == 0 ) {
				print "<tr>";
				for ( $ci = 0; $ci < $cols && (($ci + $mi) <= $#methods); $ci++ ) {
					print "<th>$methods[$mi + $ci]</th>";
				}
				print "</tr>\n<tr>";
			}
			print <<EOF;
<td><img src="$dest" width="$px" height="$py" alt="$m"></td>
EOF
			$i++;
			if ( $i == $cols ) {
				print "</tr>\n";
				$i = 0;
			}
		}
		print "</table>\n";
	}
	print <<EOF;
</body></html>
EOF
}
