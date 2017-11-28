package cmsc131.poker;

/**
 * All of the poker hands, ranked from least valuable to most valuable.
 * 
 * Poker hands are described at
 * https://en.wikipedia.org/wiki/List_of_poker_hands
 *
 */
public enum PokerHand {
  HIGH_CARD, PAIR, TWO_PAIR, THREE_OF_A_KIND, STRAIGHT, FLUSH, FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH;

  /**
   * Use the methods available in AnalyzePoker to determine the highest poker
   * hard present.
   * 
   */
  public static PokerHand get(AnalyzePoker analyze) {
    PokerHand[] possibilities = PokerHand.values();
    for (int i = possibilities.length - 1; i >= 0; i--)
      try {
        if (possibilities[i].matches(analyze))
          return possibilities[i];
      } catch (UnsupportedOperationException e) {
        // ignore
      }
    throw new RuntimeException("Impossible; HIGH_CARD should have matched");
  }

  /**
   * Verify that the analysis reports true for this poker hand. This method
   * doesn't check to see if there might be a higher ranked hand which also
   * matches.
   * 
   * By checking this, we can determine if a hand isn't properly recognized
   * because a higher ranking hand is incorrectly recognized.
   */
  public boolean matches(AnalyzePoker analyze) {
    switch (this) {
    case STRAIGHT_FLUSH:
      return analyze.hasStraightFlush();
    case STRAIGHT:
      return analyze.hasStraight();
    case FLUSH:
      return analyze.hasFlush();
    case FOUR_OF_A_KIND:
      return analyze.hasFourOfAKind();
    case FULL_HOUSE:
      return analyze.hasFullHouse();
    case THREE_OF_A_KIND:
      return analyze.hasThreeOfAKind();
    case TWO_PAIR:
      return analyze.hasTwoPair();
    case PAIR:
      return analyze.hasPair();
    case HIGH_CARD:
      return true;
    default:
      throw new RuntimeException("Unhandled poker hand: " + this);
    }
  }

}
