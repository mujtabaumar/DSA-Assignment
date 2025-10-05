# DSA-Assignment
# Project Overview

for these three programs, my main goal was to build everything from scratch but keep it simple, readable, and modular.  
i didn’t want to depend on fancy libraries or shortcuts that hide the logic.  
i wanted to actually see how things work underneath.

---

## 1. polynomial

for the polynomial, i used a `map<int, int>` to store terms where the key is the exponent and the value is the coefficient.  
i made sure every polynomial object keeps track of its own data properly, handling stuff like copies, empty cases, zero coefficients, and invalid exponents.

each operation like add, multiply, and derivative returns a new polynomial.  
i used a global storage map with a mutex so it’s thread-safe and to make sure temporary results don’t disappear when returned from functions.

basically, i wanted it to be reliable no matter what you throw at it — adding empty ones, using big exponents, or random invalid inputs.  
the idea wasn’t just to get the math right, but also to make sure it behaves cleanly and doesn’t break memory or lose data when reused.

---

## 2. uno

for uno, i just wanted to get the logic and flow right, not visuals.  
i made enums for colors and card types, then used a simple struct for each card that can describe itself like “red 5” or “green skip.”  
the whole game state keeps track of the deck, discard pile, hands, direction, and current player.

i followed proper uno rules like skips, reverses, draw twos, and reshuffling the discard pile.  
each player plays automatically based on what card can be legally played.  
i also used a fixed random seed (`1234`) when shuffling so that the results are predictable and easy to debug — no weird randomness.

it’s just a clean, rule-following simulation that plays correctly every time without depending on randomness or external libraries.

---

## 3. text editor

this one’s small but neat.  
i used two strings to represent text on the left and right of the cursor.  
the right side is stored reversed so moving the cursor or deleting characters stays efficient.

each operation like insert, delete, move left, and move right just updates those two strings.  
and i stored the state in a static map keyed by the editor’s pointer so multiple editors can exist at once without interfering with each other.

it’s basically a minimal model of how real text editors handle cursors and typing, just done with basic string juggling.

---

## overall

through all three, i kept the same mindset: simple, clear, and logical.  
no unnecessary complexity — just clean data handling, error safety, and realistic behavior.  
i wrote the comments in a casual way so if i read the code months later, i’d instantly remember what’s happening.  
the goal wasn’t to make it fancy, just to make it solid, understandable, and mine.

---

# problems i faced

some of the problems i faced while working on these three programs were actually more about logic and structure than syntax.  
i wanted everything to behave properly even in weird cases, which made debugging trickier than i expected.

## polynomial
the biggest headache here was data management.  
since i couldn’t change the header file, i had to find a way to store terms without adding class members.  
that’s why i went with a global map that uses the object’s pointer as a key.  

but that caused problems at first — especially when functions returned new polynomial objects.  
the results were showing up as “0” because the data wasn’t getting copied over properly.  
i had to figure out how to detect when a new object was created and make it “adopt” the right data.  

another issue was handling cases like adding zero coefficients or exponents repeating after operations.  
i also had to make sure nothing crashed when polynomials were empty or when users entered weird inputs.

## uno
uno looked simple at first, but there were lots of small rule-based edge cases.  
handling things like skip, reverse, and draw two correctly while keeping the turn order consistent was tricky.  
i also had to make sure the deck reshuffled properly when it ran out without losing the top discard card.  

and since the game was meant to be predictable for testing, i had to set a fixed random seed for shuffling — otherwise the results would change every run and be harder to debug.  
keeping track of the direction (clockwise or counterclockwise) and handling two-player cases where “reverse” basically becomes a “skip” also needed extra conditions.

## text editor
this one was smaller but still had its challenges.  
the main issue was figuring out how to represent the cursor efficiently.  
using two strings (left and right of the cursor) seemed easy, but i had to handle the right side in reverse so that moving the cursor didn’t require rebuilding the entire string each time.  

another small issue was keeping track of multiple text editor objects at once.  
i used a static map keyed by the object’s address to isolate their states, but at first, i accidentally shared data between editors because i didn’t handle the keys correctly.  

it took a bit of testing to make sure cursor movement, insertion, and deletion worked exactly how a normal editor would behave.

---

## final thoughts
the main problems across all three weren’t about syntax but about making everything stable and consistent.  
a lot of time went into fixing edge cases like empty data, invalid inputs, or unexpected object lifecycles.  
but each problem forced me to understand what was really happening behind the scenes — especially with object memory and logic flow.  
in the end, every fix made the code cleaner and more reliable, which was kind of the whole point of doing it from scratch.
