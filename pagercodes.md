Motorola Pager Model Number Decoder Guide
=========================================

I've pieced this together from various text files and internet posts like [this one](https://www.funkmeldesystem.de/threads/31195-Programmierproblem-Motorola-AdvisorAAA/page2), plus some of my own observations.

Some models are likely missing.  Please feel free to send a PR or get in touch to make updates.

The model numbers are of the form:

<Frequency Band><Model><Crystal Type><Alert Type><Encoding><Receiver Type><Function Type><Battery Type><Revision>

Frequency Band:
* A01 - Low Band
* A03 - VHF
* A04 - UHF
* A05 - 900 Mhz
* A06 - 900 Mhz 2-way (ReFLEX)

Model:
* AU - Digitz
* AW - Inflo
* BE - Jazz
* BP - Bravo Plus
* CJ - Keynote
* CM - Advisor Elite
* DP - OPTRX
* EJ - Advisor Pro
* FV - CP1250
* GV - Bravo Classic
* HJ - Renegade
* HN - Ultra Express
* JR - Bravo
* KL - Advisor
* LW - Bravo Encore
* MV - Bravo LX/FLX
* NK - Bravo Express
* PH - Memo Express
* PK - Talkabout T-900 
* QA - Bravo
* QT - Confidant
* QW - Advisor Gold
* TW - Pro Encore
* US - RSVP
* UW - Advisor Gold FLX
* UY - Pronto FLX
* VH - Advisor Gold FLX II
* VT - LifeStyle Plus
* VV - Free Spirit
* WT - PMR 2000

Crystal Type:
* B - Stock Frequency
* C - Any Frequency, Any Option

Alert Type:
* 1 - Tone Only
* 2 - Vibrate & Voice
* 3 - Visual & Tone
* 4 - Numeric Only
* 5 - Vibrate, Visual & Tone
* 6 - Numeric & Tone (?) (used on PMR 2000)
* 7 - Voice, Vibrate & Tone
* 8 - Voice Only
* 9 - Voice & Vibrate

Encoding:
* 1 - POCSAG 2400
* 2 - ? (not used?)
* 3 - POCSAG 1200
* 4 - ? (not used?)
* 5 - 5/6 Tone
* 6 - GSC (Golay)
* 7 - ? (not used?)
* 8 - FLEX
* 9 - POCSAG 512

Receiver Type:
* 1 - Synthesized
* 6 - Non-synthesized
* 7 - 280 Mhz receiver


Function Type:
* 1 - Numeric
* 2 - Alphanumeric
* 3 - No Display
* 4 - ? (not used?)
* 5 - ? (not used?)
* 6 - Silent Alert
* 7 - ? (not used?)
* 8 - Tone Only


Battery Type:
* A - Alkaline
* B - Alkaline (only seen on PMR 200, size N/E90/LR1)
* C - Charger
* Z - Zinc

