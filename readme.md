# Iron Phoenix

**Iron Phoenix** is a C++ chess engine built for **4-Player Chess Teams**.
The engine is designed around a fast mailbox board representation, team-based evaluation, alpha-beta search, transposition tables, and modern move-ordering techniques.

## Overview

Iron Phoenix is not a standard 2-player chess engine. It is built specifically for **4-player chess teams**, where:

- Red and Yellow are teammates.
- Blue and Green are teammates.
- Turn order is Red → Blue → Yellow → Green.
- The search evaluates positions from the current side’s team perspective.

The goal of the project is to build a strong, fast, and scalable 4PC Teams engine capable of deep search, reliable move ordering, and strong team-aware evaluation.
