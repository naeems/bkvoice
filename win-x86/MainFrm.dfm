object MainForm: TMainForm
  Left = 478
  Top = 146
  Width = 349
  Height = 497
  Caption = 'MainForm'
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -16
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 16
  object Label1: TLabel
    Left = 56
    Top = 48
    Width = 56
    Height = 16
    Caption = 'SendTo:'
  end
  object edtToAddr: TEdit
    Left = 128
    Top = 40
    Width = 121
    Height = 24
    TabOrder = 0
    Text = '192.168.2.2'
  end
  object Button1: TButton
    Left = 80
    Top = 320
    Width = 75
    Height = 25
    Caption = #21457#36865#35821#38899
    TabOrder = 1
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 184
    Top = 320
    Width = 75
    Height = 25
    Caption = #32467' '#26463
    TabOrder = 2
    OnClick = Button2Click
  end
  object Button3: TButton
    Left = 80
    Top = 368
    Width = 75
    Height = 25
    Caption = #25509#25910#35821#38899
    TabOrder = 3
    OnClick = Button3Click
  end
end
