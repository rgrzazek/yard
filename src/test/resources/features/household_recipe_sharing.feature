Feature: Household recipe sharing

  Two people who share a household should be able to share recipes privately
  with each other, without those recipes becoming visible to anyone outside
  the household — including a former member who has left but is still
  signed in.

  Scenario: A recipe stays private to a household, and stops being visible to a member who leaves — even while they're still signed in
    Given Alex has signed up and started a new household
    And Alex has generated an invite link for the household
    And Sam has signed up
    When Sam joins Alex's household using the invite link
    Then Sam is a member of Alex's household

    When Alex creates the recipe "Weeknight Bolognese"
    Then Alex can see "Weeknight Bolognese" in their recipe list
    And Alex can modify "Weeknight Bolognese"

    And Sam can see "Weeknight Bolognese" in their recipe list
    But Sam cannot modify "Weeknight Bolognese"

    When Sam leaves Alex's household
    Then Sam can no longer see "Weeknight Bolognese"
    And Alex can still see "Weeknight Bolognese" in their recipe list
