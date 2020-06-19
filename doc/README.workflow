========== BACnet Stack source code management workflow  ==========
We are working on what will be 1.0.0 in . Once 1.0.0 is finished,
branch master into a new "bacnet-stack-1.0" branch,
and create a "1.0.0" tag.  Work on what will eventually be 1.1.0 continues
in master.

When you come across some bugs in the code, fix them in master.
Then merge the fixes over to the 1.0 branch. You may also get bug
reports for 1.0.0, and fix the bugs in the 1.0 branch, and then merge
them back to master. Sometimes a bug can only be fixed in 1.0 because
it is obsolete in 1.1. It doesn't really matter, the only thing is
you want to make sure that you don't release 1.1.0 with the same bugs
that have been fixed in 1.0 branch. Once you find enough bugs
(or maybe one critical bug), you decide to do a 1.0.1 release.
So you make a tag "1.0.1" from the 1.0 branch, and release the code.
At this point, master sill contains what will be 1.1.0, and
the "1.0.0" branch contains 1.0.1 code. The next time you release an
update to 1.0 branch, it would be 1.0.2.

You can also continue to maintain 1.0 brance if you'd like, porting bug fixes
between all 3 branches (1.0.0, 1.1.0, and master). The important take
away is that for every main version of the software you are maintaining,
you have a branch that contains the latest version of code for that version.

Another use of branches is for features. This is where you branch master
(or one of your release branches) and work on a new feature in isolation.
Once the feature is completed, you merge it back in and remove the branch.
The idea of this is when you're working on something disruptive
(that would hold up or interfere with other people from doing their work),
something experimental (that may not even make it in), or possibly just
something that takes a long time (and you're afraid if it holding up a
1.2.0 release when you're ready to branch 1.2.0 from master), you can do
it in isolation in branch. Generally you keep it up to date with master by
merging changes into it all the time, which makes it easier to re-integrate
(merge back to master) when you're finished.
