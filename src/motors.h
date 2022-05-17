

void move(int zadana_X, int zadana_Y)
{

  zadana_Y = perpendicular_Ytracker_degree(zadana_Y);

  if (zadana_X < T_XMIN) // sprawdzenie czy zadany kąt nie wybiega za kąt obrotu trackera
  {
    zadana_X = T_XMIN;
  }
  if (zadana_X > T_XMAX)
  {
    zadana_X = T_XMAX;
  }
  if (zadana_Y < T_YMIN)
  {
    zadana_Y = T_YMIN;
  }
  if (zadana_Y > T_YMAX)
  {
    zadana_Y = T_YMAX;
  }

  if (Xangle + delta_ON < zadana_X) // na lewo od zadanej poz
  {
    digitalWrite(E_motor, HIGH);
    digitalWrite(W_motor, LOW);
    stop_X = true;
  }

  if (Xangle - delta_ON > zadana_X) // na prawo od zadanej poz
  {
    digitalWrite(E_motor, LOW);
    digitalWrite(W_motor, HIGH);
    stop_X = true;
  }

  if (abs(Xangle - zadana_X) < delta_ON)
  {
    if (Xangle < zadana_X && stop_X == false)
    {
      digitalWrite(E_motor, HIGH);
      digitalWrite(W_motor, LOW);
    }

    if (Xangle > zadana_X && stop_X == false)
    {
      digitalWrite(E_motor, LOW);
      digitalWrite(W_motor, HIGH);
    }

    if (abs(Xangle - zadana_X) <= delta_OFF)
    {
      stop_X = true;
      digitalWrite(E_motor, LOW);
      digitalWrite(W_motor, LOW);
    }
  }

  //---------------- OS Y --------------//

  if (Yangle + delta_ON < zadana_Y) // na lewo od zadanej poz
  {
    digitalWrite(N_motor, HIGH);
    digitalWrite(S_motor, LOW);
    stop_Y = true;
  }

  if (Yangle - delta_ON > zadana_Y) // na prawo od zadanej poz
  {
    digitalWrite(N_motor, LOW);
    digitalWrite(S_motor, HIGH);
    stop_Y = true;
  }

  if (abs(Yangle - zadana_Y) < delta_ON)
  {
    if (Yangle < zadana_Y && stop_Y == false)
    {
      digitalWrite(N_motor, HIGH);
      digitalWrite(S_motor, LOW);
    }

    if (Yangle > zadana_Y && stop_Y == false)
    {
      digitalWrite(N_motor, LOW);
      digitalWrite(S_motor, HIGH);
    }

    if (abs(Yangle - zadana_Y) <= delta_OFF)
    {
      stop_Y = true;
      digitalWrite(N_motor, LOW);
      digitalWrite(S_motor, LOW);
    }
  }
}

void manualny()
{

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("manualny:         ");

  while (true)
  {
    key = kpd.getKey();
    state = kpd.getState();

    get_pot_value();
    lcd.setCursor(0, 1);
    lcd.print("X:");
    lcd.print(Xangle);
    lcd.print("  ");
    lcd.setCursor(8, 1);
    lcd.print("Y:");
    lcd.print(Yangle);
    lcd.print("  ");

    while (key == '2')
    {
      kpd.getKey(); // puste pobranie znaku bo bez tego nie zadziała pobranie  getState()
      state = kpd.getState();

      get_pot_value();
      lcd.setCursor(0, 1);
      lcd.print("X:");
      lcd.print(Xangle);
      lcd.print("  ");
      lcd.setCursor(8, 1);
      lcd.print("Y:");
      lcd.print(Yangle);
      lcd.print("  ");

      while (Yangle < T_YMAX - delta)
      {
        digitalWrite(N_motor, HIGH);
        break;
      }

      while (Yangle >= T_YMAX - delta)
      {
        digitalWrite(N_motor, LOW);
        break;
      }

      if (state == 3)
      {
        key = ' ';
      }
    }

    while (key == '8')
    {
      kpd.getKey();           // puste pobranie znaku bo bez tego
      state = kpd.getState(); // nie zadziała pobranie getState()

      get_pot_value();
      lcd.setCursor(0, 1);
      lcd.print("X:");
      lcd.print(Xangle);
      lcd.print("  ");
      lcd.setCursor(8, 1);
      lcd.print("Y:");
      lcd.print(Yangle);
      lcd.print("  ");

      while (Yangle > T_YMIN + delta)
      {
        digitalWrite(S_motor, HIGH);
        break;
      }

      while (Yangle <= T_YMIN + delta)
      {
        digitalWrite(S_motor, LOW);
        break;
      }

      if (state == 3)
      {
        key = ' ';
      }
    }

    while (key == '4')
    {
      kpd.getKey();
      state = kpd.getState();

      get_pot_value();
      lcd.setCursor(0, 1);
      lcd.print("X:");
      lcd.print(Xangle);
      lcd.print("  ");
      lcd.setCursor(8, 1);
      lcd.print("Y:");
      lcd.print(Yangle);
      lcd.print("  ");

      while (Xangle > T_XMIN + delta)
      {
        digitalWrite(W_motor, HIGH);
        break;
      }

      while (Xangle <= T_XMIN + delta)
      {
        digitalWrite(W_motor, LOW);
        break;
      }

      if (state == 3)
      {
        key = ' ';
      }
    }

    while (key == '6')
    {
      kpd.getKey();
      state = kpd.getState();

      get_pot_value();
      lcd.setCursor(0, 1);
      lcd.print("X:");
      lcd.print(Xangle);
      lcd.print("  ");
      lcd.setCursor(8, 1);
      lcd.print("Y:");
      lcd.print(Yangle);
      lcd.print("  ");

      while (Xangle < T_XMAX - delta)
      {
        digitalWrite(E_motor, HIGH);
        break;
      }

      while (Xangle >= T_XMAX - delta)
      {
        digitalWrite(E_motor, LOW);
        break;
      }

      if (state == 3)
      {
        key = ' ';
      }
    }

    if (state == 3)
    {
      digitalWrite(N_motor, LOW);
      digitalWrite(S_motor, LOW);
      digitalWrite(W_motor, LOW);
      digitalWrite(E_motor, LOW);
    }

    if (key == '*')
    {
      lcd.clear();
      break;
    }
    if (key == 'B')
    {
      manualnyBIS();
    }
    if (key == 'D')
    {
      menu_item();
    }
  }
}

void manualnyBIS()
{

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("manualny dlugi");
  lcd.setCursor(0, 1);
  lcd.print("5-stop   *-wyjdz");

  while (true)
  {
    key = kpd.getKey();
    state = kpd.getState();

    if (key == '2')
    {
      flag1 = !flag1;
      digitalWrite(N_motor, flag1);
    }
    if (digitalRead(N_motor) == HIGH && digitalRead(S_motor) == HIGH)
    {
      digitalWrite(N_motor, LOW);
    }

    if (key == '8')
    {
      flag2 = !flag2;
      digitalWrite(S_motor, flag2);
    }
    if (digitalRead(N_motor) == HIGH && digitalRead(S_motor) == HIGH)
    {
      digitalWrite(S_motor, LOW);
    }

    if (key == '4')
    {
      flag3 = !flag3;
      digitalWrite(W_motor, flag3);
    }
    if (digitalRead(W_motor) == HIGH && digitalRead(E_motor) == HIGH)
    {
      digitalWrite(W_motor, LOW);
    }
    if (key == '6')
    {
      flag4 = !flag4;
      digitalWrite(E_motor, flag4);
    }
    if (digitalRead(W_motor) == HIGH && digitalRead(E_motor) == HIGH)
    {
      digitalWrite(E_motor, LOW);
    }
    if (key == '*')
    {
      lcd.clear();
      break;
    }

    if (key == 'A')
    {
      manualny();
    }
    if (key == 'D')
    {
      menu_item();
    }
    if (key == '5')
    {
      digitalWrite(N_motor, LOW);
      digitalWrite(S_motor, LOW);
      digitalWrite(W_motor, LOW);
      digitalWrite(E_motor, LOW);
    }
  }
}

void tracker_move_calculate()

{

  //---ZNAJDOWANIE NAJWIEJSZYCH I NAJMNIEJSZYCH MOZLIWYCH WYCHYŁOW TRACKERA---

  if (Xangle_min > tracker_Xmin)
  {
    T_XMIN = Xangle_min;
  }
  else if (Xangle_min < tracker_Xmin)
  {
    T_XMIN = tracker_Xmin;
  }
  else if (Xangle_min == tracker_Xmin)
  {
    T_XMIN = Xangle_min;
  }

  if (Xangle_max < tracker_Xmax)
  {
    T_XMAX = Xangle_max;
  }
  else if (Xangle_max > tracker_Xmax)
  {
    T_XMAX = tracker_Xmax;
  }
  else if (Xangle_max == tracker_Xmax)
  {
    T_XMAX = Xangle_max;
  }

  if (Yangle_min > tracker_Ymin)
  {
    T_YMIN = Yangle_min;
  }
  else if (Yangle_min < tracker_Ymin)
  {
    T_YMIN = tracker_Ymin;
  }
  else if (Yangle_min == tracker_Ymin)
  {
    T_YMIN = Yangle_min;
  }

  if (Yangle_max < tracker_Ymax)
  {
    T_YMAX = Yangle_max;
  }
  else if (Yangle_max > tracker_Ymax)
  {
    T_YMAX = tracker_Ymax;
  }
  else if (Yangle_max == tracker_Ymax)
  {
    T_YMAX = Yangle_max;
  }
}

int perpendicular_Ytracker_degree(int kat_slonca)
{
  return 90 - kat_slonca;
}