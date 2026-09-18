#include "DropMapLabelWidget.h"
#include "Components/TextBlock.h"

void UDropMapLabelWidget::SetLabelText(const FText& Text)
{
	if (LabelText)
	{
		LabelText->SetText(Text);
	}
}
