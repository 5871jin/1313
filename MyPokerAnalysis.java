package cmsc131.poker;

import java.util.Collection;

import cmsc131.cards.Card;

/**
 * Having analyzed a set of poker cards, determine what poker hands it contains.
 * Note: if you haven't yet implemented a method, have you can either just
 * return false or throw UnsupportedOperationException.
 *
 */
public class MyPokerAnalysis implements AnalyzePoker {

  public MyPokerAnalysis(Collection<Card> cards) {
    throw new UnsupportedOperationException("Replace with your implementation");

  }

  @Override
  public boolean hasPair() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasThreeOfAKind() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasTwoPair() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasFourOfAKind() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasFullHouse() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasStraight() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasFlush() {
    // TODO Auto-generated method stub
    return false;
  }

  @Override
  public boolean hasStraightFlush() {
    // TODO Auto-generated method stub
    return false;
  }

}
